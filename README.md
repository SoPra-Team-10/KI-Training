[<img src="https://travis-ci.org/SoPra-Team-10/KI-Training.svg?branch=master" alt="Build Status">](https://travis-ci.org/SoPra-Team-10/KI-Training)
# KI-Training
Component for training the AI.

## Usage
Pass the match config json file as the first argument.

## Getting started
You can choose between using Docker or manually installing all dependencies.
Docker is the preferred method as it already installs the toolchain
and all dependencies.

### Docker
In the root directory of the project build the docker image
("kitraining" is the name of the container, this can be replaced by a
different name):
```
docker build -t kitraining .
```

Now start the container, you need to map the
external file (match.json) to an internal file:
```
docker run -v $(pwd)/match.json:match.json kitraining ./KiTraining /match.json
```
That's it you should now have a running docker instance.

### Manually installing the Server
If you need to debug the program it can be easier to do this outside
of docker.

### Prerequisites
 * A C++17 compatible Compiler (e.g. GCC-8)
 * CMake (min 3.10) and GNU-Make
 * Adress-Sanitizer for run time checks
 * [SopraGameLogic](https://github.com/SoPra-Team-10/GameLogic)
 * [SopraMessages](https://github.com/SoPra-Team-10/Messages)
 * [SopraUtil](https://github.com/SoPra-Team-10/Util)
 * [MLP](https://github.com/aul12/MLP)
 * Either a POSIX-Compliant OS or Cygwin (to use pthreads)
 * Optional: Google Tests and Google Mock for Unit-Tests

### Compiling the Application
In the root directory of the project create a new directory
(in this example it will be called build), change in this directory.

Next generate a makefile using cmake:
```
cmake ..
```
if any error occurs recheck the prerequisites. Next compile the program:
```
make
```
you can now run the program by executing the created `KiTraining` file:
```
./KiTraining
```



## External Librarys
 * [SopraGameLogic](https://github.com/SoPra-Team-10/GameLogic)
 * [SopraMessages](https://github.com/SoPra-Team-10/Messages)
 * [SopraUtil](https://github.com/SoPra-Team-10/Util)
 * [MLP](https://github.com/aul12/MLP)
 * [nlohmann::json](https://github.com/nlohmann/json)

## Doxygen Dokumentation
- [Master Branch](https://sopra-team-10.github.io/KI-Training/master/html/index.html)
- [Develop Branch](https://sopra-team-10.github.io/KI-Training/develop/html/index.html)

## SonarQube Analyse
Das Analyseergebniss von SonarQube ist [hier auf SonarCloud](https://sonarcloud.io/dashboard?id=SoPra-Team-10_KI-Training) zu finden.
