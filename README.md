# mscp

[![build on ubuntu](https://github.com/upa/mscp/actions/workflows/build-ubuntu.yml/badge.svg)](https://github.com/upa/mscp/actions/workflows/build-ubuntu.yml) [![build on macOS](https://github.com/upa/mscp/actions/workflows/build-macos.yml/badge.svg)](https://github.com/upa/mscp/actions/workflows/build-macos.yml) [![test](https://github.com/upa/mscp/actions/workflows/test.yml/badge.svg)](https://github.com/upa/mscp/actions/workflows/test.yml)


`mscp`, a variant of `scp`, copies files over multiple ssh (SFTP)
sessions. Multiple threads in mscp transfer (1) multiple files
simultaneously and (2) a large file in parallel. It may shorten the
waiting time for transferring a lot of/large files over networks.

You can use `mscp` like `scp`, for example, `mscp
user@example.com:srcfile /tmp/dstfile`. Remote hosts only need to run
standard `sshd` supporting the SFTP subsystem, and you need to be able
to ssh to the hosts (as usual). `mscp` does not require anything else.


Differences from `scp` are:

- remote glob on remote shell expansion is not supported.
- remote to remote copy is not supported.
- `-r` option is not needed.
- and any other differences I have not implemented and noticed...


## Install

- homebrew

```console
brew install upa/tap/mscp
```

- Linux

Download a package for your environment from [Releases
page](https://github.com/upa/mscp/releases).


## Build from source

mscp depends on [libssh](https://www.libssh.org/).

- macOS

```console
brew install libssh
```

- ubuntu

```console
sudo apt-get install libssh-dev
```

- rhel

```console
sudo yum install libssh-devel
```

Clone and build this repositoy.

```console
git clone https://github.com/upa/mscp.git
cd mscp

mkdir build && cd build
cmake .. && make

# install the mscp binary to CMAKE_INSTALL_PREFIX/bin (usually /usr/local/bin)
make install
```

## Run

- Usage

```console
$ mscp
mscp v0.0.0: copy files over multiple ssh connections

Usage: mscp [vqDCHdh] [-n nr_conns]
            [-s min_chunk_sz] [-S max_chunk_sz]
            [-b sftp_buf_sz] [-B io_buf_sz] [-a nr_ahead]
            [-l login_name] [-p port] [-i identity_file]
            [-c cipher_spec] source ... target
```

- Example: copy an 8GB file on tmpfs over a 100Gbps link
  - Two Intel Xeon Gold 6130 machines directly connected with Intel E810 100Gbps NICs.

```console
$ mscp /tmp/test.img 10.0.0.1:/tmp/
[=====================================================] 100% 8GB/8GB 3.02GB/s 
```

- `-v` option increments verbose output level.

```console
$ mscp test 10.0.0.1:
[=====================================================] 100% 13B/13B 2.41KB/s 

$ mscp -v test 10.0.0.1:
file test/test.txt (local) -> ./test/test.txt (remote) 9B
file test/test2/2.txt (local) -> ./test/test2/2.txt (remote) 2B
file test/1.txt (local) -> ./test/1.txt (remote) 2B
copy start: test/test.txt
copy start: test/1.txt
copy start: test/test2/2.txt
copy done: test/1.txt
copy done: test/test2/2.txt
copy done: test/test.txt
[=====================================================] 100% 13B/13B 2.51KB/s 

$ mscp -vv -n 4 test 10.0.0.1:
connecting to 10.0.0.1 for checking destinations...
file test/test.txt (local) -> ./test/test.txt (remote) 9B
file test/test2/2.txt (local) -> ./test/test2/2.txt (remote) 2B
file test/1.txt (local) -> ./test/1.txt (remote) 2B
connecting to 10.0.0.1 for a copy thread...
connecting to 10.0.0.1 for a copy thread...
connecting to 10.0.0.1 for a copy thread...
connecting to 10.0.0.1 for a copy thread...
copy start: test/test.txt
copy start: test/1.txt
copy start: test/test2/2.txt
copy done: test/test.txt
copy done: test/test2/2.txt
copy done: test/1.txt
[=====================================================] 100% 13B/13B 3.27KB/s
```

- Full usage

```console
$ mscp -h
mscp v0.0.0: copy files over multiple ssh connections

Usage: mscp [vqDCHdh] [-n nr_conns]
            [-s min_chunk_sz] [-S max_chunk_sz]
            [-b sftp_buf_sz] [-B io_buf_sz] [-a nr_ahead]
            [-l login_name] [-p port] [-i identity_file]
            [-c cipher_spec] source ... target

    -n NR_CONNECTIONS  number of connections (default: half of # of cpu cores)
    -s MIN_CHUNK_SIZE  min chunk size (default: 64MB)
    -S MAX_CHUNK_SIZE  max chunk size (default: filesize / nr_conn)

    -b SFTP_BUF_SIZE   buf size for sftp_read/write (default 131072B)
    -B IO_BUF_SIZE     buf size for read/write (default 131072B)
                       Note that the default value is derived from
                       qemu/block/ssh.c. need investigation...
                       -b and -B affect only local to remote copy
    -a NR_AHEAD        number of inflight SFTP read commands (default 16)

    -v                 increment verbose output level
    -q                 disable output
    -D                 dry run

    -l LOGIN_NAME      login name
    -p PORT            port number
    -i IDENTITY        identity file for publickey authentication
    -c CIPHER          cipher spec, see `ssh -Q cipher`
    -C                 enable compression on libssh
    -H                 disable hostkey check
    -d                 increment ssh debug output level
    -h                 print this help
```


Note: mscp is still under development, and the author is not
responsible for any accidents due to mscp.

## Build with Async Write

```console
# install required package
sudo apt install libkrb5-dev

# clone this repositoy
git clone https://github.com/upa/mscp -b async-write
cd mscp

# build pathched libssh
git clone https://git.libssh.org/projects/libssh.git/ --depth=10 -b libssh-0.10.4
cd libssh && git apply ../patch/libssh-0.10.4.patch
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=../../libssh-installed
make && make install

# build mscp with patched libssh
# mv to mscp dir
mv ../..
mkdir build && cd build
cmake .. -DLIBSSH_PATH=../libssh-installed
make
```