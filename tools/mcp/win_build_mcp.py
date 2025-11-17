import asyncio
import os
import json
import shlex
import subprocess
from mcp.server import Server

server = Server("win-build-mcp")

@server.tool("find_msbuild", "Find MSBuild path")
async def find_msbuild() -> str:
    p = r"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
    if os.path.exists(p):
        try:
            out = subprocess.check_output([p, "-latest", "-products", "*", "-requires", "Microsoft.Component.MSBuild", "-find", "MSBuild\\**\\Bin\\MSBuild.exe"], text=True).strip()
            return out
        except Exception:
            return ""
    return ""

@server.tool("build_solution", "Build a solution")
async def build_solution(path: str, configuration: str = "Release") -> str:
    msbuild = await find_msbuild()
    if not msbuild:
        return json.dumps({"error": "MSBuild not found"})
    proc = await asyncio.create_subprocess_exec(msbuild, path, f"/p:Configuration={configuration}", "/m", stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.PIPE)
    out, err = await proc.communicate()
    return json.dumps({"exit_code": proc.returncode, "stdout": out.decode(errors="ignore"), "stderr": err.decode(errors="ignore")})

@server.tool("build_project", "Build a vcxproj")
async def build_project(path: str, configuration: str = "Release") -> str:
    msbuild = await find_msbuild()
    if not msbuild:
        return json.dumps({"error": "MSBuild not found"})
    proc = await asyncio.create_subprocess_exec(msbuild, path, f"/p:Configuration={configuration}", "/m", stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.PIPE)
    out, err = await proc.communicate()
    return json.dumps({"exit_code": proc.returncode, "stdout": out.decode(errors="ignore"), "stderr": err.decode(errors="ignore")})

@server.tool("run_ctest", "Run ctest in build dir")
async def run_ctest(build_dir: str, configuration: str = "Release") -> str:
    cmd = ["ctest", "-C", configuration]
    proc = await asyncio.create_subprocess_exec(*cmd, stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.PIPE, cwd=build_dir)
    out, err = await proc.communicate()
    return json.dumps({"exit_code": proc.returncode, "stdout": out.decode(errors="ignore"), "stderr": err.decode(errors="ignore")})

@server.tool("run_exe", "Run executable")
async def run_exe(path: str, args: str = "", timeout_sec: int = 30) -> str:
    argv = [path] + (shlex.split(args) if args else [])
    proc = await asyncio.create_subprocess_exec(*argv, stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.PIPE)
    try:
        out, err = await asyncio.wait_for(proc.communicate(), timeout=timeout_sec)
        return json.dumps({"exit_code": proc.returncode, "stdout": out.decode(errors="ignore"), "stderr": err.decode(errors="ignore")})
    except asyncio.TimeoutError:
        try:
            proc.kill()
        except ProcessLookupError:
            pass
        return json.dumps({"error": "timeout"})

async def main():
    await server.run_stdio_server()

if __name__ == "__main__":
    asyncio.run(main())