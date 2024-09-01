Framework that combines global system monitorization with application level monitorization. The objective is to schedule and manage the whole cluster to exploit the available resources reducing the application's execution time. 


Configuration parameters for the different components: 

Limitless Daemon Monitor
-i : time interval (1000)
-s : number of samples
-a : destination IP
-[z,k] : backup IP destinations
-p : port
-n : net reduction (0 disabled, 1 enabled)
-[t]: tolerance
-b : bitmap (0 disabled, 1 enabled)
-m : 0 TMR 1-1, 1 TMR 3-3


Limitless Daemon Server
-p : port for LDM
-c : port for Communication-API
-f : port for FlexMpi
-[q,w,e] : thresholds
-n : 0 for server, 1 if LDA enabled
-t : heartbeat time
-m : 0 TMR 1-1, 1 TMR 3-3


Limitless Daemon Aggregator
-p : port for lower level daemon/retransmitter
-[y,u] : backup upper level component (retransitter/server)
-r : master port
-s : Master IP
-n : 1 (retransmitter enabled)


Connection API
-b : DB IP
-s : Server/LDA IP
-p : Server/LDA port
-[y,u]: Server/LDA backup IPs

