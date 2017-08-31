# simple.Unix.Shell
A simple shell program written in C using POSIX libraries.


nsh: a simple shell


nsh may be ran:

As an interactive shell by calling it with no arguments.
  
  
On a script by providing the script name as the first argument.
  

nsh supports the following basic shell abilities:


1. command execution no arguments


2. command execution with arguments


3. input redirection <


4. output redirection >


5. Multiple pipes


7. error detection + handling


8. cd builtin command


9. nsh scripts (avoid printing prompt and read commands from file listed on command line)


10. Comments

	

Known issues:


commands that normally require the use of singlequotes '' for arguments will not be parsed correctly with them.
		for most cases, providing the argument without the singlequote will typically result in the command being run as expected 
    Example: 
        
        find /usr/bin/ | 'sed s:.*/::' | grep -i '^a' > ../out1bash.txt ## will not be parsed/ran correctly
        find /usr/bin/ | sed s:.*/:: | grep -i ^a > ../out1bash.txt ## will be parsed and will output the same as bash
