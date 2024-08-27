sleep(600);
fulload() { dd if=/dev/zero of=/dev/null | dd if=/dev/zero of=/dev/null | dd if=/dev/zero of=/dev/null | dd if=/dev/zero of=/dev/null | dd if=/dev/zero of=/dev/null  | dd if=/dev/zero of=/dev/null  | dd if=/dev/zero of=/dev/null  & }; fulload; read; killall dd
sleep(5);
#start variation

#copy and paste dd line between curly brackets, one for each core. 
