# Installations (Linux - ubuntu)
```bash
sudo apt install clang
wget https://apt.llvm.org/llvm.sh
```
- This project used llvm-20 , version 20.1.8

```bash
sudo ./llvm.sh 20
```

- Note that the LLVM version number is written in the CMAKELISTS of ./analysis folder
- Similar LLVM versions may run. However the python script will also need to modified along with the cmakelists if so
- clang-20 and opt-20 commands are used in the python script, and version number is written in the cmakelists in the ./analysis folder

``` bash
sudo apt install libzstd-dev # (maybe required)
pip install -r requirements.txt
```

## Further setup and usage
- Run build.sh in the analysis folder

```bash
python live_var_app.py
```

- Open on browser with 127.0.0.1:8050 address

# Using the dockerfile
- Ensure your docker engine is running
```bash
docker build . -t live-var-ui:1.0
```
- On git bash run the following. (Should be similar for other shells/terminals)
```bash
docker run -it -p 8050:8050 -v "/$(pwd):/app" live-var-ui:1.0
```
- If you already run once, you can just start the same container using the container id. Find the container id with docker ps command.
```bash
docker ps -a
docker start -ai <container_id>
```
- In the bash of the container run the start shell script which will compile the analysis pass and run the python flask server.
```bash
./start.sh
```
- Access the server from web browser at 127.0.0.1:8050
- Clean after usage is done
```bash
make clean
```
