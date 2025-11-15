#include "NeonGlyph.h"
#include "Application.h"
#include "AIDirector.h"
#include "StoryContext.h"
#include "MusicAnalyzer.h"
#include <iostream>
#include <iomanip>
#include <memory>
#include <chrono>
#include <thread>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace NeonGlyph;

class StoryDemo {
public:
    StoryDemo() {
        m_director = std::make_unique<AIDirector>();
        m_music_analyzer = std::make_unique<MusicAnalyzer>();
        m_start_time = std::chrono::steady_clock::now();
    }
    
    Result Initialize() {
        // Initialize director
        Config config;
        config.llm.enabled = false; // Disable LLM for demo
        Result result = m_director->Initialize(config);
        if (result != Result::Success) {
            std::cout << "Failed to initialize AIDirector" << std::endl;
            return result;
        }
        
        // Load story set
        std::string story_json = LoadStoryFile("config/story_neon_koi.json");
        if (story_json.empty()) {
            std::cout << "Failed to load story file, using default story" << std::endl;
            CreateDefaultStory();
        } else {
            result = m_director->LoadStorySet(story_json);
            if (result != Result::Success) {
                std::cout << "Failed to load story set, using default" << std::endl;
                CreateDefaultStory();
            }
        }
        
        // Initialize music analyzer
        result = m_music_analyzer->Initialize(config);
        if (result != Result::Success) {
            std::cout << "Failed to initialize MusicAnalyzer" << std::endl;
            return result;
        }
        
        std::cout << "Story Demo initialized successfully!" << std::endl;
        std::cout << "Story system will guide visual evolution over 60 minutes." << std::endl;
        std::cout << "The story follows: Arrival -> Exploration -> Transformation -> Resolution" << std::endl;
        
        return Result::Success;
    }
    
    void RunDemo() {
        std::cout << "\n=== Starting Story Demo ===" << std::endl;
        std::cout << "Generating visual commands based on story arcs..." << std::endl;
        std::cout << "Press Ctrl+C to stop.\n" << std::endl;
        
        int frame_count = 0;
        auto last_report_time = std::chrono::steady_clock::now();
        
        while (true) {
            // Generate mock music data that changes over time
            MusicData music_data = GenerateMockMusicData(frame_count);
            
            // Get director command with story integration
            DirectorCommand cmd = m_director->Update(music_data);
            
            // Get current story context
            const StoryContext& story = m_director->GetStoryContext();
            
            // Print status every 5 seconds
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_report_time).count();
            if (elapsed >= 5) {
                PrintStatus(story, cmd, frame_count);
                last_report_time = now;
            }
            
            // Simulate frame processing
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            frame_count++;
            
            // Stop after 60 minutes (3600 seconds) of simulated time
            if (frame_count * 0.1f >= 3600.0f) {
                std::cout << "\n=== Story Demo Complete ===" << std::endl;
                break;
            }
        }
    }
    
    void Shutdown() {
        if (m_director) m_director->Shutdown();
        if (m_music_analyzer) m_music_analyzer->Shutdown();
    }

private:
    std::unique_ptr<AIDirector> m_director;
    std::unique_ptr<MusicAnalyzer> m_music_analyzer;
    std::chrono::steady_clock::time_point m_start_time;
    
    std::string LoadStoryFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return "";
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        return content;
    }
    
    void CreateDefaultStory() {
        // Create a simple default story if file loading fails
        std::string simple_story = R"({
            "set_id": "default_demo_story",
            "total_duration_sec": 3600,
            "acts": [
                {
                    "id": "act_1",
                    "title": "Beginning",
                    "time_range_sec": [0, 900],
                    "theme_family": ["gentle", "introductory"],
                    "scenes": [
                        {
                            "id": "scene_1",
                            "title": "Opening",
                            "relative_range": [0.0, 1.0],
                            "intensity_curve": "0.2->0.5",
                            "dominant_palettes": ["soft_colors"]
                        }
                    ]
                }
            ]
        })";
        
        m_director->LoadStorySet(simple_story);
    }
    
    MusicData GenerateMockMusicData(int frame_count) {
        MusicData data;
        
        // Simulate 60 minutes of music progression
        float simulated_time_seconds = frame_count * 0.1f; // 100ms per frame
        float normalized_time = simulated_time_seconds / 3600.0f; // 0 to 1 over 60 minutes
        
        // Generate music data that follows the story structure
        if (normalized_time < 0.25f) {
            // Act 1: Arrival - gentle, building
            data.song_section = "intro";
            data.bpm = 80 + static_cast<int>(normalized_time * 40);
            data.energy = 0.2f + normalized_time * 0.3f;
            data.dynamics = 0.3f + normalized_time * 0.2f;
            data.beat_phase = (frame_count % 80) / 80.0f; // Slow beat
            data.rms_level_db = -25.0f;
            data.key = "A minor";
        } else if (normalized_time < 0.5f) {
            // Act 2: Exploration - steady, flowing
            data.song_section = "verse";
            data.bpm = 100 + static_cast<int>((normalized_time - 0.25f) * 40);
            data.energy = 0.5f + (normalized_time - 0.25f) * 0.3f;
            data.dynamics = 0.5f + (normalized_time - 0.25f) * 0.2f;
            data.beat_phase = (frame_count % 60) / 60.0f; // Medium beat
            data.rms_level_db = -20.0f;
            data.key = "C major";
        } else if (normalized_time < 0.75f) {
            // Act 3: Transformation - intense, building to climax
            data.song_section = "chorus";
            data.bpm = 120 + static_cast<int>((normalized_time - 0.5f) * 40);
            data.energy = 0.8f + (normalized_time - 0.5f) * 0.2f;
            data.dynamics = 0.7f + (normalized_time - 0.5f) * 0.3f;
            data.beat_phase = (frame_count % 40) / 40.0f; // Fast beat
            data.rms_level_db = -15.0f;
            data.key = "E major";
        } else {
            // Act 4: Resolution - gentle decline
            data.song_section = "bridge";
            data.bpm = 140 - static_cast<int>((normalized_time - 0.75f) * 80);
            data.energy = 1.0f - (normalized_time - 0.75f) * 0.7f;
            data.dynamics = 1.0f - (normalized_time - 0.75f) * 0.5f;
            data.beat_phase = (frame_count % 100) / 100.0f; // Slowing beat
            data.rms_level_db = -30.0f + (normalized_time - 0.75f) * 15.0f;
            data.key = "F major";
        }
        
        // Add some silence periods to test blackout functionality
        if (frame_count % 1000 == 0 && frame_count > 0) {
            data.rms_level_db = -60.0f; // Simulate silence
        }
        
        return data;
    }
    
    void PrintStatus(const StoryContext& story, const DirectorCommand& cmd, int frame_count) {
        float simulated_time_minutes = (frame_count * 0.1f) / 60.0f;
        
        std::cout << "\n--- Story Status Update ---" << std::endl;
        std::cout << "Simulated Time: " << std::fixed << std::setprecision(1) << simulated_time_minutes << " minutes" << std::endl;
        
        if (story.act) {
            std::cout << "Current Act: " << story.act->title << " (" << story.act->id << ")" << std::endl;
            std::cout << "  Theme Family: ";
            for (const auto& theme : story.act->theme_family) {
                std::cout << theme << " ";
            }
            std::cout << std::endl;
            std::cout << "  Arc Shape: " << story.act->arc_shape << std::endl;
        }
        
        if (story.scene) {
            std::cout << "Current Scene: " << story.scene->title << " (" << story.scene->id << ")" << std::endl;
            std::cout << "  Scene Phase: " << std::fixed << std::setprecision(2) << story.scene_phase << std::endl;
            std::cout << "  Intensity Curve: " << story.scene->intensity_curve << std::endl;
            std::cout << "  Dominant Palette: " << story.scene->dominant_palettes[0] << std::endl;
        }
        
        std::cout << "Visual Command:" << std::endl;
        std::cout << "  Scene ID: " << cmd.scene_id << std::endl;
        std::cout << "  Palette: " << cmd.mise_en_scene.palette << std::endl;
        std::cout << "  Brightness: " << std::fixed << std::setprecision(2) << cmd.mise_en_scene.brightness << std::endl;
        std::cout << "  Contrast: " << cmd.mise_en_scene.contrast << std::endl;
        std::cout << "  Motion Speed: " << cmd.motion.speed << std::endl;
        std::cout << "  Transition: " << cmd.transition.type << " (" << cmd.transition.duration_ms << "ms)" << std::endl;
        
        if (story.is_blackout) {
            std::cout << "  *** BLACKOUT STATE - Audio Silence Detected ***" << std::endl;
        }
        
        std::cout << "Novelty Status:" << std::endl;
        // In a real implementation, we'd expose the novelty detector stats
        std::cout << "  Visual fingerprint tracking active (preventing repetition)" << std::endl;
    }
};

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    std::cout << "=== NeonGlyph Story System Demo ===" << std::endl;
    std::cout << "This demo shows the story-driven visual evolution system." << std::endl;
    std::cout << "The system will guide visual generation through 4 acts over 60 minutes." << std::endl;
    std::cout << "Features: story arcs, scene transitions, novelty detection, silence blackout.\n" << std::endl;
    
    StoryDemo demo;
    
    if (demo.Initialize() != Result::Success) {
        std::cout << "Failed to initialize demo" << std::endl;
        return 1;
    }
    
    try {
        demo.RunDemo();
    } catch (const std::exception& e) {
        std::cout << "Demo error: " << e.what() << std::endl;
    }
    
    demo.Shutdown();
    return 0;
}
