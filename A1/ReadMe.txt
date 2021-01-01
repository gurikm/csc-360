
steps to run and stop code:
1. Type "make" to compile code.
2. Type "./Pman" to excute code.
3. Type "bg" "./<cmd>" and any required variables to run background process
4. Type "exit" to end program.

possible commands to type to test code:

1. bg cmd: starts <cmd> running in the background
2. bgkill pid:will send the TERM signal to the job with process ID pid to terminate that job.
3. bgstop pid:will send the STOP signal to the job pid to stop (temporarily) that job.
4. bgstart pid:will send the CONT signal to the job pid to re-start that job (which has been previously stopped).
5. bglist: will have PMan display a list of all the programs currently executing in the background,
6. pstat pid:list the following information related to process pid,where pid is the Process ID:comm,state,utime,stime,rss,voluntary_ctxt_switch,non-voluntary_ctxt_switch
