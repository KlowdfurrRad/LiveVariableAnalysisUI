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

# Further setup and usage
- Run build.sh in the analysis folder

```bash
python live_var_app.py
```

- Open on browser with 127.0.0.1:8050 address