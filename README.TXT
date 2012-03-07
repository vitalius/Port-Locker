On a unix system, ports above 1024 are a fair game to grab by any user or application. Sometimes there is competition for certain ports and this program (pssrv), if loaded before any other programs will attempt to lock TCP/UDP ports specified in pssvr.config file. 

The locked ports can then be released, or new ones added, as needed by the psc (client) program on a remote machine.