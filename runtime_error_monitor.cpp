#include <iostream>
#include <string>
#include <stdexcept>
#include <sstream>
#include <windows.h>
#include <dbghelp.h>
#include <vector>

#pragma comment(lib, "dbghelp.lib")

class RuntimeErrorMonitor {
private:
    static RuntimeErrorMonitor* instance;
    std::vector<std::string> errorLog;
    
    RuntimeErrorMonitor() {
        // Set up unhandled exception filter
        SetUnhandledExceptionFilter(UnhandledExceptionHandler);
        
        // Set up terminate handler
        std::set_terminate(TerminateHandler);
        
        // Set up unexpected handler
        std::set_unexpected(UnexpectedHandler);
    }
    
    static LONG WINAPI UnhandledExceptionHandler(EXCEPTION_POINTERS* ExceptionInfo) {
        std::cerr << "[RuntimeErrorMonitor] FATAL: Unhandled exception occurred!" << std::endl;
        std::cerr << "[RuntimeErrorMonitor] Exception code: 0x" << std::hex << ExceptionInfo->ExceptionRecord->ExceptionCode << std::endl;
        
        // Print stack trace
        PrintStackTrace(ExceptionInfo->ContextRecord);
        
        // Log error details
        if (instance) {
            std::stringstream ss;
            ss << "Unhandled exception: 0x" << std::hex << ExceptionInfo->ExceptionRecord->ExceptionCode;
            instance->errorLog.push_back(ss.str());
        }
        
        return EXCEPTION_EXECUTE_HANDLER;
    }
    
    static void TerminateHandler() {
        std::cerr << "[RuntimeErrorMonitor] FATAL: terminate() called!" << std::endl;
        
        // Get current exception if any
        try {
            throw;
        } catch (const std::invalid_argument& e) {
            std::cerr << "[RuntimeErrorMonitor] std::invalid_argument: " << e.what() << std::endl;
            if (instance) {
                instance->errorLog.push_back(std::string("std::invalid_argument: ") + e.what());
            }
        } catch (const std::exception& e) {
            std::cerr << "[RuntimeErrorMonitor] std::exception: " << e.what() << std::endl;
            if (instance) {
                instance->errorLog.push_back(std::string("std::exception: ") + e.what());
            }
        } catch (...) {
            std::cerr << "[RuntimeErrorMonitor] Unknown exception type" << std::endl;
            if (instance) {
                instance->errorLog.push_back("Unknown exception type");
            }
        }
        
        std::exit(1);
    }
    
    static void UnexpectedHandler() {
        std::cerr << "[RuntimeErrorMonitor] FATAL: unexpected() called!" << std::endl;
        
        try {
            throw;
        } catch (const std::invalid_argument& e) {
            std::cerr << "[RuntimeErrorMonitor] std::invalid_argument: " << e.what() << std::endl;
            if (instance) {
                instance->errorLog.push_back(std::string("std::invalid_argument: ") + e.what());
            }
        } catch (const std::exception& e) {
            std::cerr << "[RuntimeErrorMonitor] std::exception: " << e.what() << std::endl;
            if (instance) {
                instance->errorLog.push_back(std::string("std::exception: ") + e.what());
            }
        } catch (...) {
            std::cerr << "[RuntimeErrorMonitor] Unknown exception type" << std::endl;
            if (instance) {
                instance->errorLog.push_back("Unknown exception type");
            }
        }
        
        std::exit(1);
    }
    
    static void PrintStackTrace(PCONTEXT context) {
        std::cerr << "[RuntimeErrorMonitor] Stack trace:" << std::endl;
        
        HANDLE process = GetCurrentProcess();
        HANDLE thread = GetCurrentThread();
        
        // Initialize symbol handler
        SymInitialize(process, NULL, TRUE);
        
        // Set up stack frame
        STACKFRAME64 stackFrame;
        memset(&stackFrame, 0, sizeof(STACKFRAME64));
        
        #ifdef _M_IX86
        DWORD machineType = IMAGE_FILE_MACHINE_I386;
        stackFrame.AddrPC.Offset = context->Eip;
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Offset = context->Ebp;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
        stackFrame.AddrStack.Offset = context->Esp;
        stackFrame.AddrStack.Mode = AddrModeFlat;
        #elif _M_X64
        DWORD machineType = IMAGE_FILE_MACHINE_AMD64;
        stackFrame.AddrPC.Offset = context->Rip;
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Offset = context->Rbp;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
        stackFrame.AddrStack.Offset = context->Rsp;
        stackFrame.AddrStack.Mode = AddrModeFlat;
        #endif
        
        // Walk the stack
        int frameCount = 0;
        while (StackWalk64(machineType, process, thread, &stackFrame, context, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL)) {
            if (frameCount++ > 20) break; // Limit stack depth
            
            // Get symbol info
            char symbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
            PSYMBOL_INFO symbol = (PSYMBOL_INFO)symbolBuffer;
            symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
            symbol->MaxNameLen = MAX_SYM_NAME;
            
            DWORD64 displacement = 0;
            if (SymFromAddr(process, stackFrame.AddrPC.Offset, &displacement, symbol)) {
                std::cerr << "[RuntimeErrorMonitor]   " << symbol->Name << " + 0x" << std::hex << displacement << std::endl;
            }
        }
        
        SymCleanup(process);
    }
    
public:
    static RuntimeErrorMonitor& GetInstance() {
        if (!instance) {
            instance = new RuntimeErrorMonitor();
        }
        return *instance;
    }
    
    void LogError(const std::string& error) {
        errorLog.push_back(error);
        std::cerr << "[RuntimeErrorMonitor] " << error << std::endl;
    }
    
    void LogStofError(const std::string& str, const std::string& context) {
        std::stringstream ss;
        ss << "STOF_ERROR: Failed to convert string '" << str << "' to float in context: " << context;
        LogError(ss.str());
    }
    
    const std::vector<std::string>& GetErrorLog() const {
        return errorLog;
    }
    
    void PrintErrorSummary() {
        std::cerr << "\n[RuntimeErrorMonitor] Error Summary:" << std::endl;
        std::cerr << "[RuntimeErrorMonitor] Total errors: " << errorLog.size() << std::endl;
        
        int stofErrors = 0;
        int otherErrors = 0;
        
        for (const auto& error : errorLog) {
            if (error.find("STOF_ERROR") != std::string::npos) {
                stofErrors++;
            } else {
                otherErrors++;
            }
        }
        
        std::cerr << "[RuntimeErrorMonitor] String conversion errors: " << stofErrors << std::endl;
        std::cerr << "[RuntimeErrorMonitor] Other errors: " << otherErrors << std::endl;
        
        if (!errorLog.empty()) {
            std::cerr << "[RuntimeErrorMonitor] Recent errors:" << std::endl;
            int startIdx = std::max(0, (int)errorLog.size() - 5);
            for (int i = startIdx; i < errorLog.size(); i++) {
                std::cerr << "[RuntimeErrorMonitor]   " << errorLog[i] << std::endl;
            }
        }
    }
};

// Static instance
RuntimeErrorMonitor* RuntimeErrorMonitor::instance = nullptr;

// Global error handlers
void InstallRuntimeErrorMonitor() {
    RuntimeErrorMonitor::GetInstance();
    std::cout << "[RuntimeErrorMonitor] Runtime error monitoring installed" << std::endl;
}

void LogStofError(const std::string& str, const std::string& context) {
    RuntimeErrorMonitor::GetInstance().LogStofError(str, context);
}

void LogRuntimeError(const std::string& error) {
    RuntimeErrorMonitor::GetInstance().LogError(error);
}

void PrintRuntimeErrorSummary() {
    RuntimeErrorMonitor::GetInstance().PrintErrorSummary();
}