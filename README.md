# peace 
A simple OpenDHT example.

## Build instructions
1. Follow the [build and install instructions](https://github.com/savoirfairelinux/opendht/wiki/Build-the-library) of OpenDHT to install the library on your system.  
On a Debian based system this could look like this:
   ```
     # Install deps
   > sudo apt install libncurses5-dev libreadline-dev nettle-dev libgnutls28-dev libargon2-0-dev libmsgpack-dev  libssl-dev libfmt-dev libjsoncpp-dev libhttp-parser-dev libasio-dev
   
     # clone the repo
   > git clone https://github.com/savoirfairelinux/opendht.git

     # build and install
   > cd opendht
   > mkdir build && cd build
   > cmake -DCMAKE_INSTALL_PREFIX=/usr ..
   > make -j4
   > sudo make install
   ```
   
2. Clone this repository and change into its root folder.
   ```
   git clone https://github.com/ElCap1tan/peace.git && cd peace
   ```
   
3. Create the build folder and change into it.
   ```
   mkdir build && cd build
   ```
   
4. Build it.
   ```
   > cmake ..
   > make -j4
   ```
   
5. The compiled executable now can be found in the build folder. To run it use `./peace`.