import subprocess

# Fixed parameters
exe_name = "./clientPacketGen"    # Executable name  
server_ip = "10.2.1.44"         # Server IP
server_port = "5555"            # Server Port
payload = "100"                 # Initial Payload

ttl_values = [2, 4, 8, 16]
file_paths = [f"TTL_{ttl}.csv" for ttl in ttl_values]

# Standard process execution steps
for ttl, file_path in zip(ttl_values, file_paths):
    command = [exe_name, server_ip, server_port, payload, str(ttl), file_path]
    
    result = subprocess.run(command, capture_output=True, text=True)
    
    if result.returncode == 0:
        print(f"Command executed successfully for TTL={ttl} and FilePath={file_path}\n")
        print("Output:\n", result.stdout)
    else:
        print(f"Error executing command for TTL={ttl} and FilePath={file_path}\n")
        print("Error:\n", result.stderr)


