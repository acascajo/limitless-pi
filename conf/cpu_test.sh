fulload() { dd if=/dev/zero of=/dev/null  & }; fulload; read; killall dd
#copy and paste dd line between curly brackets, one for each core. 
