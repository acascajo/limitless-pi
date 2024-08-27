#include <iostream>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <math.h>
#include <string>
#include <vector>
#include "Packed_sample.hpp"
#include "common.hpp"
using namespace std;


Packed_sample::Packed_sample(){

}


Packed_sample::Packed_sample(Hw_conf hwconf, unsigned int interval, int n_samples, int threshold) {
    this->ip_addr_s.assign(hwconf.ip_addr_s);
    this->n_samples = n_samples;
    this->modo_bitmap = hwconf.modo_bitmap;
    //cout << "Obtenido el modo bitmap: " << this->modo_bitmap  << endl;
    this->n_devices_io = hwconf.n_devices_io;
    this->memtotal = hwconf.mem_total;
    this->n_interfaces = hwconf.n_interfaces;
    this->n_core_temps = hwconf.n_core_temps;
    this->threshold = threshold;
    this->n_cpu = hwconf.n_cpu;
    this->n_cores = hwconf.n_cores;
    this->interval = interval;
    this->n_gpus = hwconf.n_gpu;
    //this->n_siblings = hwconf.n_siblings;
    time(&(this->time_sample));
    samples_packed = 0;
    sample_pt = 0;
    packed_bytes = 0;
    datos_cuartil = 0;
    packed_ptr = 0;
    calculate_bit_maps_bytes();
    this->bit_map = std::unique_ptr<unsigned char[]>(new unsigned char[bytes_bitmap]);
    pack_features();
    calculate_sample_size();
    pos_bitmap = 0;
    pos_sample = 0;
    //calculate_bit_maps_bytes();

    //sample_size = 0;

    for (int i = 0; i < n_devices_io; i++) {
        dev_avg.push_back(0);

    }

    for (int i = 0; i < n_interfaces; i++) {
        net_avg.push_back(0);
    }

    for (int i = 0; i < n_core_temps; i++) {
        temp_avg.push_back(0);
    }

    for (int i = 0; i < n_gpus; i++) {
        gpu_avg.push_back(0);
    }

    /*for (int i = 0; i < n_gpus; i++) {

        Gpu_accu gpuaccu;

        gpuaccu.memUsage = 0;
        gpuaccu.gpuUsage = 0;
        gpuaccu.temperature = 0;
        gpuaccu.powerUsage = 0;

        gpu_accu.push_back(gpuaccu);
    }*/

    for (int i = 0; i < n_cpu; i++) {
        pw_cpu.push_back(0);
    }

}

Packed_sample::~Packed_sample() {

    /* Clear vector */
    dev_avg.clear();
    net_avg.clear();
    temp_avg.clear();
    gpu_avg.clear();
    //gpu_accu.clear();

/*            
            		vector<unsigned char>().swap(dev_avg);
            		vector<unsigned char>().swap(net_avg);
	            	vector<unsigned char>().swap(temp_avg);
*/
}

/*
 *	Pack the features in the packed_buffer
 */
void Packed_sample::pack_features() {

    /* Packing ip address. Position:0-3 */
#ifdef DEBUG_DAEMON
    //cout << "antes de parsear" << endl;
    //cout << ip_addr_s << endl;
#endif

    ip_v = parse_log(ip_addr_s);

#ifdef DEBUG_DAEMON
    //cout << "ip parseada" << endl;
#endif


    for (int i = 0; i < ip_v.size(); i++) {

#ifdef DEBUG_DAEMON
        //cout << "ip addr: " << ip_v[i] << endl;
#endif

        packed_buffer[i] = (unsigned char) stoi(ip_v[i]);
        packed_ptr++;
    }

    /* Packing memory total. Position:4 */
#ifdef DEBUG_DAEMON
    //cout << memtotal << endl;
#endif

    packed_buffer[packed_ptr] = memtotal;
    packed_ptr++;

#ifdef DEBUG_DAEMON
    //cout << "n_cpus: "<<n_cpu << endl;
#endif

    /* Packing number of cpus. Position:5 */
    packed_buffer[packed_ptr] = (unsigned char) n_cpu;
    packed_ptr++;

    /* Packing number of cores. Position:6 */
    packed_buffer[packed_ptr] = (unsigned char) n_cores;
    packed_ptr++;

    /* Packing number of devices. Position:7 */
    packed_buffer[packed_ptr] = (unsigned char) n_devices_io;
    packed_ptr++;

    /* Packing number of interfaces. Position:8 */
    packed_buffer[packed_ptr] = (unsigned char) n_interfaces;
    packed_ptr++;

    /* Packing number of core_temps. Position:9 */
    packed_buffer[packed_ptr] = (unsigned char) n_core_temps;
    packed_ptr++;

    /* Packing number of gpus. Position:10 */
    packed_buffer[packed_ptr] = (unsigned char) n_gpus;
    packed_ptr++;

    /* Packing number of samples. Position:11 */
    packed_buffer[packed_ptr] = (unsigned char) n_samples;
    packed_ptr++;

    /* Guardar modo bitmap */
    //packed_buffer[packed_ptr] = (unsigned char)modo_bitmap;
    //packed_ptr++;

    /* Packing number of bytes of bitmap. Position:12 */
    //packed_buffer[packed_ptr] = (unsigned char)bytes_bitmap;
    //packed_ptr++;

    /* Guardar espacio para los bytes del bitmap */
    //pos_bitmap = packed_ptr;

    //cout << "Posicion del puntero: "<< packed_ptr << endl;
    //cout << "Numero de bytes es: "<< bytes_bitmap << endl;
    //packed_ptr = packed_ptr + bytes_bitmap;
    //cout << "Posicion del puntero despues: "<< packed_ptr << endl;
    //pos_sample = packed_ptr;
    //cout << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++posicion en la que empiezan los datos es: "<< packed_ptr << endl;
}


//		void Packed_sample::pack_monitoring(int mem_usa, int cpu_usa, int * devices_usa[2], int * net_usa, int * temp){
//
//			packed_buffer[10] = mem_usa;
//
//			packed_buffer[11] = cpu_usa;
//
//			for(int i = 0; i < n_devices_io; i++){
//				packed_buffer[12] = devices_usa[i][0];
//				packed_buffer[12 + 1 ] = devices_usa[i][1];
//			}
//
//		}



		/*
		****************************************************************************************************************
		* Pack sample into buffer to be sent.
		* IN:
		* @sample String to be packed in the buffer.
		****************************************************************************************************************
		*/

void Packed_sample::pack_sample_s(string sample, double cache_miss, double cpu_stalled){//, std::vector<Core_dev> cores) {
            double dtmp = 0.0;
            std::vector<std::string> svec = split(sample, ' ');

            int sample_concat = 0;
            int stringptr = 0;

#ifdef DEBUG_DAEMON
            cout << "samples packed " << samples_packed << "  n samples "<< n_samples << endl;
            cout << "string sample " << sample << endl;
#endif


            /* Si se han empaquetado el numero de muestras que se quiere, se debe crear el mapa de bits y enviar los datos */
            if (samples_packed == n_samples) {
#ifdef DEBUG_LBBM
                //cout << "enviar ->> : " << endl;
                //cout << packed_ptr << endl;

                for (int i = 0; i < packed_ptr; i++) {
                    //printf("%d ", packed_buffer[i]);
                }

                //cout << endl;

#endif

                //memset(&(bit_map),0,bytes_bitmap);

                /* Generar el bitmap y guardarlo en el packed buffer */
                to_bit_map();
                packed_ptr = 0;
                pack_features();
                sample_pt = 0;
                samples_packed = 0;

                time(&(this->time_sample));
            }
            if (packed_bytes == 0) {

                sample_pt = 0;
                sample_concat = 0;

            } else {

                sample_concat = 12;

            }

            stringptr = 2;

#ifdef DEBUG_DAEMON
            cout << "Sample size is: " << n_samples << endl;
    cout << "Temperature is: " << n_core_temps << endl;

    cout << "memory usage: " << stoi(svec[stringptr]) << endl;
    cout << "se pone en la posicion: " << packed_ptr << endl;
    cout << "sample concat: " << sample_concat << endl;
#endif

            /* Packing memory usage */
            packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
            packed_ptr++;
            sample_concat++;
            packed_bytes++;
            stringptr += 3;

            /* Packing cpu idle */
#ifdef DEBUG_DAEMON
            cout << "cpu busy: " << stod(svec[stringptr]) << endl;
            cout << "se pone en la posicion: " << packed_ptr << endl;
            cout << "sample concat: " << sample_concat << endl;
#endif
            dtmp = stod(svec[stringptr]);
            packed_buffer[packed_ptr] = (unsigned char) dtmp;
            packed_ptr++;
            sample_concat++;
            packed_bytes++;
            stringptr++;


            /* Packing  energy usage for each cpu */
            for (int i = 0; i < n_cpu; i++) {

#ifdef DEBUG_DAEMON
                cout << "power usage cpu (Joules): " << stod(svec[stringptr]) << endl;
                cout << "se pone en la posicion: " << packed_ptr << endl;
                cout << "sample concat: " << sample_concat << endl;
#endif
                dtmp = stod(svec[stringptr]);
                packed_buffer[packed_ptr] = (unsigned char) dtmp;
                packed_ptr++;
                packed_bytes++;
                sample_concat++;
                stringptr++;

            }


            /* Packing  w(%) and tIO(%) for each device */
            for (int i = 0; i < n_devices_io; i++) {

#ifdef DEBUG_DAEMON
                cout << "w(%): " << stoi(svec[stringptr]) << endl;
                cout << "se pone en la posicion: " << packed_ptr << endl;
                cout << "sample concat: " << sample_concat << endl;
#endif

                packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
                packed_ptr++;
                packed_bytes++;
                sample_concat++;
                stringptr++;

#ifdef DEBUG_DAEMON
                cout << "tIO(%): " << stoi(svec[stringptr]) << endl;
                cout << "se pone en la posicion: " << packed_ptr<< endl;
                cout << "sample concat: " << sample_concat << endl;
#endif

                packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
                packed_ptr++;
                packed_bytes++;
                sample_concat++;
                stringptr++;

            }

            /* Packing speed of the network and network usage (%) for each interface*/
            for (int i = 0; i < n_interfaces; i++) {

#ifdef DEBUG_DAEMON
                cout << "Gb/s: " << stoi(svec[stringptr]) << endl;
                cout << "se pone en la posicion: " << packed_ptr<< endl;
                cout << "sample concat: " << sample_concat << endl;
#endif

                packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
                packed_ptr++;
                packed_bytes++;
                sample_concat++;
                stringptr++;


#ifdef DEBUG_DAEMON
                cout << "netusage(%): " << stoi(svec[stringptr]) << endl;
                cout << "se pone en la posicion: " << packed_ptr << endl;
        cout << "sample concat: " << sample_concat << endl;
#endif

                packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
                packed_ptr++;
                packed_bytes++;
                sample_concat++;
                stringptr++;

            }

            /* Packing temperature and temp(%) for each coretemp */
            for (int i = 0; i < n_core_temps; i++) {

#ifdef DEBUG_DAEMON
                cout << "temperature: " << stoi(svec[stringptr]) << endl;
        cout << "se pone en la posicion: " << packed_ptr << endl;
        cout << "sample concat: " << sample_concat << endl;
#endif

                packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
                packed_ptr++;
                packed_bytes++;
                sample_concat++;
                stringptr++;

#ifdef DEBUG_DAEMON
                cout << "temperature(%): " << stoi(svec[stringptr]) << endl;
                cout << "se pone en la posicion: " << packed_ptr << endl;
                cout << "sample concat: " << sample_concat << endl;
#endif

                packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
                packed_ptr++;
                packed_bytes++;
                sample_concat++;
                stringptr++;

            }

            /* Packing gpu features: memory usage(%), gpu usage(%), temp(degrees Celsius) and powerUsage(Watts) for each gpu */
            for (int i = 0; i < n_gpus; i++) {

//                    		#ifdef DEBUG_DAEMON
//                        		cout << "cuda compatible: " << stoi(svec[stringptr]) << endl;
//    					cout << "se pone en la posicion: " << packed_ptr << endl;
//    					cout << "sample concat: " << sample_concat << endl;
//        			#endif

//    				packed_buffer[packed_ptr] =  (unsigned char) stoi(svec[stringptr]);
                //packed_ptr++;
                //packed_bytes++;
                //sample_concat++;
                //stringptr++;

#ifdef DEBUG_DAEMON
                cout << "memory usage(%): " << stoi(svec[stringptr]) << endl;
                cout << "se pone en la posicion: " << packed_ptr << endl;
                cout << "sample concat: " << sample_concat << endl;
#endif

                packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
                packed_ptr++;
                packed_bytes++;
                sample_concat++;
                stringptr++;

#ifdef DEBUG_DAEMON
                cout << "gpu usage(%): " << stoi(svec[stringptr]) << endl;
                cout << "se pone en la posicion: " << packed_ptr << endl;
                cout << "sample concat: " << sample_concat << endl;
#endif

                packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
                packed_ptr++;
                packed_bytes++;
                sample_concat++;
                stringptr++;

#ifdef DEBUG_DAEMON
                cout << "temperatura (degrees Celsius): " << stoi(svec[stringptr]) << endl;
                cout << "se pone en la posicion: " << packed_ptr << endl;
                cout << "sample concat: " << sample_concat << endl;
#endif

                packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
                packed_ptr++;
                packed_bytes++;
                sample_concat++;
                stringptr++;

#ifdef DEBUG_DAEMON
                cout << "power usage: " << stoi(svec[stringptr]) << endl;
                cout << "se pone en la posicion: " << packed_ptr << endl;
                cout << "sample concat: " << sample_concat << endl;
#endif

                packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
                packed_ptr++;
                packed_bytes++;
                sample_concat++;
                stringptr++;
            }


#ifdef DEBUG_DAEMON
            cout << "Packed_buffer" << endl;
                for(int i = 0; i < packed_ptr; i++){
                printf("---%d---\n",packed_buffer[i]);
            }
#endif

            /*PACK CACHE INFORMATION*/
            int r = (int)cache_miss;
            packed_buffer[packed_ptr] = (unsigned char) r;
            packed_ptr++;
            packed_bytes++;
            sample_concat++;
            stringptr++;

            /*PACK cpu stalled  INFORMATION*/
            int s = (int)cpu_stalled;
            packed_buffer[packed_ptr] = (unsigned char) s;
            packed_ptr++;
            packed_bytes++;
            sample_concat++;
            stringptr++;


            /*PACK core information*/
            /*packed_buffer[packed_ptr] = (unsigned char) n_siblings;
            packed_ptr++;
            packed_bytes++;
            sample_concat++;
            stringptr++;
            for(int core = 0; core < cores.size(); ++core) {
                int l = (int)cores[core].load;
                packed_buffer[packed_ptr] = (unsigned char)l;
                packed_ptr++;
                packed_bytes++;
                sample_concat++;
                stringptr++;
            }
            n_siblings = (int)cores.size();
		*/

            samples_packed++;
        }

void Packed_sample::pack_sample_s(string sample) {
    double dtmp = 0.0;
    std::vector<std::string> svec = split(sample, ' ');

    int sample_concat = 0;
    int stringptr = 0;

#ifdef DEBUG_DAEMON
    cout << "samples packed " << samples_packed << "  n samples "<< n_samples << endl;
            cout << "string sample " << sample << endl;
#endif


    /* Si se han empaquetado el numero de muestras que se quiere, se debe crear el mapa de bits y enviar los datos */
    if (samples_packed == n_samples) {
#ifdef DEBUG_LBBM
        //cout << "enviar ->> : " << endl;
        //cout << packed_ptr << endl;

        for (int i = 0; i < packed_ptr; i++) {
            printf("%d ", packed_buffer[i]);
        }

        cout << endl;

#endif

        //memset(&(bit_map),0,bytes_bitmap);

        /* Generar el bitmap y guardarlo en el packed buffer */
        to_bit_map();
        packed_ptr = 0;
        pack_features();
        sample_pt = 0;
        samples_packed = 0;

        time(&(this->time_sample));
    }
    if (packed_bytes == 0) {

        sample_pt = 0;
        sample_concat = 0;

    } else {

        sample_concat = 12;

    }

    stringptr = 2;

#ifdef DEBUG_DAEMON
    cout << "Sample size is: " << n_samples << endl;
    cout << "Temperature is: " << n_core_temps << endl;

    cout << "memory usage: " << stoi(svec[stringptr]) << endl;
    cout << "se pone en la posicion: " << packed_ptr << endl;
    cout << "sample concat: " << sample_concat << endl;
#endif

    /* Packing memory usage */
    packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
    packed_ptr++;
    sample_concat++;
    packed_bytes++;
    stringptr += 3;

    /* Packing cpu idle */
#ifdef DEBUG_DAEMON
    cout << "cpu busy: " << stod(svec[stringptr]) << endl;
            cout << "se pone en la posicion: " << packed_ptr << endl;
            cout << "sample concat: " << sample_concat << endl;
#endif
    dtmp = stod(svec[stringptr]);
    packed_buffer[packed_ptr] = (unsigned char) dtmp;
    packed_ptr++;
    sample_concat++;
    packed_bytes++;
    stringptr++;


    /* Packing  energy usage for each cpu */
    for (int i = 0; i < n_cpu; i++) {

#ifdef DEBUG_DAEMON
        cout << "power usage cpu (Joules): " << stod(svec[stringptr]) << endl;
                cout << "se pone en la posicion: " << packed_ptr << endl;
                cout << "sample concat: " << sample_concat << endl;
#endif
        dtmp = stod(svec[stringptr]);
        packed_buffer[packed_ptr] = (unsigned char) dtmp;
        packed_ptr++;
        packed_bytes++;
        sample_concat++;
        stringptr++;

    }


    /* Packing  w(%) and tIO(%) for each device */
    for (int i = 0; i < n_devices_io; i++) {

#ifdef DEBUG_DAEMON
        cout << "w(%): " << stoi(svec[stringptr]) << endl;
                cout << "se pone en la posicion: " << packed_ptr << endl;
                cout << "sample concat: " << sample_concat << endl;
#endif

        packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
        packed_ptr++;
        packed_bytes++;
        sample_concat++;
        stringptr++;

#ifdef DEBUG_DAEMON
        cout << "tIO(%): " << stoi(svec[stringptr]) << endl;
                cout << "se pone en la posicion: " << packed_ptr<< endl;
                cout << "sample concat: " << sample_concat << endl;
#endif

        packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
        packed_ptr++;
        packed_bytes++;
        sample_concat++;
        stringptr++;

    }

    /* Packing speed of the network and network usage (%) for each interface*/
    for (int i = 0; i < n_interfaces; i++) {

#ifdef DEBUG_DAEMON
        cout << "Gb/s: " << stoi(svec[stringptr]) << endl;
                cout << "se pone en la posicion: " << packed_ptr<< endl;
                cout << "sample concat: " << sample_concat << endl;
#endif

        packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
        packed_ptr++;
        packed_bytes++;
        sample_concat++;
        stringptr++;


#ifdef DEBUG_DAEMON
        cout << "netusage(%): " << stoi(svec[stringptr]) << endl;
                cout << "se pone en la posicion: " << packed_ptr << endl;
        cout << "sample concat: " << sample_concat << endl;
#endif

        packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
        packed_ptr++;
        packed_bytes++;
        sample_concat++;
        stringptr++;

    }

    /* Packing temperature and temp(%) for each coretemp */
    for (int i = 0; i < n_core_temps; i++) {

#ifdef DEBUG_DAEMON
        cout << "temperature: " << stoi(svec[stringptr]) << endl;
        cout << "se pone en la posicion: " << packed_ptr << endl;
        cout << "sample concat: " << sample_concat << endl;
#endif

        packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
        packed_ptr++;
        packed_bytes++;
        sample_concat++;
        stringptr++;

#ifdef DEBUG_DAEMON
        cout << "temperature(%): " << stoi(svec[stringptr]) << endl;
                cout << "se pone en la posicion: " << packed_ptr << endl;
                cout << "sample concat: " << sample_concat << endl;
#endif

        packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
        packed_ptr++;
        packed_bytes++;
        sample_concat++;
        stringptr++;

    }

    /* Packing gpu features: memory usage(%), gpu usage(%), temp(degrees Celsius) and powerUsage(Watts) for each gpu */
    for (int i = 0; i < n_gpus; i++) {

//                    		#ifdef DEBUG_DAEMON
//                        		cout << "cuda compatible: " << stoi(svec[stringptr]) << endl;
//    					cout << "se pone en la posicion: " << packed_ptr << endl;
//    					cout << "sample concat: " << sample_concat << endl;
//        			#endif

//    				packed_buffer[packed_ptr] =  (unsigned char) stoi(svec[stringptr]);
        //packed_ptr++;
        //packed_bytes++;
        //sample_concat++;
        //stringptr++;

#ifdef DEBUG_DAEMON
        cout << "memory usage(%): " << stoi(svec[stringptr]) << endl;
                cout << "se pone en la posicion: " << packed_ptr << endl;
                cout << "sample concat: " << sample_concat << endl;
#endif

        packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
        packed_ptr++;
        packed_bytes++;
        sample_concat++;
        stringptr++;

#ifdef DEBUG_DAEMON
        cout << "gpu usage(%): " << stoi(svec[stringptr]) << endl;
                cout << "se pone en la posicion: " << packed_ptr << endl;
                cout << "sample concat: " << sample_concat << endl;
#endif

        packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
        packed_ptr++;
        packed_bytes++;
        sample_concat++;
        stringptr++;

#ifdef DEBUG_DAEMON
        cout << "temperatura (degrees Celsius): " << stoi(svec[stringptr]) << endl;
                cout << "se pone en la posicion: " << packed_ptr << endl;
                cout << "sample concat: " << sample_concat << endl;
#endif

        packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
        packed_ptr++;
        packed_bytes++;
        sample_concat++;
        stringptr++;

#ifdef DEBUG_DAEMON
        cout << "power usage: " << stoi(svec[stringptr]) << endl;
                cout << "se pone en la posicion: " << packed_ptr << endl;
                cout << "sample concat: " << sample_concat << endl;
#endif

        packed_buffer[packed_ptr] = (unsigned char) stoi(svec[stringptr]);
        packed_ptr++;
        packed_bytes++;
        sample_concat++;
        stringptr++;
    }


#ifdef DEBUG_DAEMON
    cout << "Packed_buffer" << endl;
                for(int i = 0; i < packed_ptr; i++){
                printf("---%d---\n",packed_buffer[i]);
            }
#endif

    samples_packed++;
}


/*
****************************************************************************************************************
* Splits the log using dots, spaces and tab characters.
* IN:
* @string	String representing the log content.
* RETURNS:
* Vector of strings with the splitted contents.
****************************************************************************************************************
*/
vector <string> Packed_sample::split_log(string str){

        if(str.length()==0){
            vector<string> internal;
    	}

        vector<string> internal;
        std::stringstream ss(str); // Turn the string into a stream.
        string tok;
        while(getline(ss, tok, '.')) {
            std::stringstream ss2(tok);
            string tok2;
            
            while(getline(ss2,tok2,' ')){

                std::stringstream ss3(tok2);
                string tok3;
                
                while(getline(ss3,tok3,'\t')){
                    
                    if(tok != ""){
                    
                        internal.push_back(tok3);
                    
                    }
                }


            }
        }

        return internal;

}

/*
****************************************************************************************************************
* Parse sample to include on the buffer
* IN:
* @sample	String including sample to be parsed.
* RETURNS:
* Vector of strings including the parsed contents.
****************************************************************************************************************
*/
vector <string> Packed_sample::parse_log(string sample){

    vector<string> log_vector;

    stringstream sdouble;

    string sinsert = "";

    /* Getting the ip of the sample */
    vector<string> sample_split = split(sample, ' ');
    string ip_ = sample_split[0];

    log_vector = split_log(ip_);

    for(int i = 1; i < sample_split.size(); i++ ){

        sdouble.str("");

        sdouble << stoi(sample_split[i]);


        log_vector.push_back(sdouble.str());
    }

    return log_vector;

}


void Packed_sample::calculate_sample_size(){
	/**  
		n_cpu = Number of cpus
		mem_cpu = 
		n_interfaces = number of interfaces
		n_devices_io = number of I/O devices
		n_core_temps = number of coretemps

		n_gpus = number of gpus
			for each gpu 4 bytes: memoryUsage, gpuUsage, temperature and powerUsage
		cache
	    stalled
	    core_load*n_core

	*/
	
	this->sample_size = 2 + n_cpu + 2 * n_devices_io + 2 * n_interfaces + 2 * n_core_temps + (4 * n_gpus) + 1 + 1 + 1;// + n_siblings; //cache and stalled
	
	#ifdef DEBUG_DAEMON
		//cout << "sample size *" << sample_size <<"*" << endl;
	#endif


}

int Packed_sample::calculate_bit_maps_bytes(){

		/**  
			Datos que se guardan en el bitmap son:
			- uso memoria
			- uso cpu
			- por cada cpu temperatura
			- por cada io device %
			- por cada interfaz %
			- por cada coretemp %

			n_cpu = Number of cpus
			mem_cpu = 
			n_interfaces = number of interfaces
			n_devices_io = number of I/O devices
			n_core_temps = number of coretemps

			n_gpus = number of gpus
				for each gpu 4 bytes: memoryUsage, gpuUsage, temperature and powerUsage 

		 */
		int total_bits = 2 + 2 + 2*(n_cpu) + 2*(n_devices_io) + 2*(n_interfaces) + 2*(n_core_temps) + 2*4*(n_gpus); //Total bits used: mem, cpu, temp for each cpu

            	//bytes_bitmap = n_cpu + (mem_cpu + n_interfaces + n_devices_io + n_core_temps) * 2 + (n_gpus * 4);
            	//bytes_bitmap = ceil((double)bytes_bitmap / 8);
            	bytes_bitmap = ceil((double)total_bits / 8);
           
		#ifdef DEBUG_LBBM
	 			cout << "------------el numero de bytes para el bit map es: "<< bytes_bitmap << endl;
                #endif

            
}


void Packed_sample::to_bit_map(){

    	/* Inicializar las variables */
    	//bit_map[bytes_bitmap];
            
    	memory_avg = 0;
    	cpu_avg = 0;
    	byte_bit_map = 0;
    	bit_map_ptr = 0;

    	int packed_buffer_ptr = 0;

    	for(int i = 0; i < n_devices_io; i++){
        	dev_avg[i] = 0;	
    	}
            
 	for(int i = 0; i < n_interfaces; i++){
        	net_avg[i] = 0;	
    	}

    	for(int i = 0; i < n_core_temps; i++){
        	temp_avg[i] = 0;	
    	}

    	for(int i = 0; i < n_gpus; i++){
		gpu_avg[i] = 0;
    	}

	for(int i = 0; i < n_gpus; i++){
		gpu_avg[i] = 0;
    	}
    

    	packed_buffer_ptr = pos_sample;

    	/* Media de los valores del packed buffer */
    	/* Primero se suman los valores acumulados en el packed buffer */
    	for(int i = 0; i < n_samples;i++){


        	//packed_buffer_ptr = 11;
        	/* Sumar los valores de la memoria */
        
        	#ifdef DEBUG_BIT_MAP
            		printf("se suma a la memoria el valor: %d\n",packed_buffer[packed_buffer_ptr]);
        	#endif
                
        	memory_avg += packed_buffer[packed_buffer_ptr];
        	packed_buffer_ptr++;

        	/* Sumar los valores de cpu */
        	#ifdef DEBUG_BIT_MAP
            		printf("se suma a la cpu el valor: %d\n",packed_buffer[packed_buffer_ptr]);
        	#endif          
                
        	cpu_avg += packed_buffer[packed_buffer_ptr];
        	packed_buffer_ptr++;

		/* Sumar los valores del uso de energia en las cpu */
        	for(int j = 0;j < n_cpu; j++){
					
            		#ifdef DEBUG_BIT_MAP
                		printf("se suma a uso de las energias de cpu el valor: %d\n",packed_buffer[packed_buffer_ptr]);           
            		#endif                    
            		pw_cpu[j] += packed_buffer[packed_buffer_ptr];
            		packed_buffer_ptr++;	
        	}

        	/* Sumar los valores de los dispositivos */
        	for(int j = 0; j < n_devices_io; j++){
            		packed_buffer_ptr++;		
					
            		#ifdef DEBUG_BIT_MAP
                		printf("se suma a los dispositivos el valor: %d\n",packed_buffer[packed_buffer_ptr]);           
            		#endif
                    
            		dev_avg[j] += packed_buffer[packed_buffer_ptr];
            		packed_buffer_ptr++;	
        	}

        	/* Sumar los valores de las interfaces */
        	for(int j = 0; j < n_interfaces; j++){
            		packed_buffer_ptr++;
            
            		#ifdef DEBUG_BIT_MAP
                		printf("se suma a las interfaces el valor: %d\n",packed_buffer[packed_buffer_ptr]);
            		#endif

            		net_avg[j] += packed_buffer[packed_buffer_ptr];
            		packed_buffer_ptr++;	
        	}

        	/* Sumar los valores de las temperaturas */
        	for(int j = 0;j < n_core_temps; j++){
            		packed_buffer_ptr++;
                    
            		#ifdef DEBUG_BIT_MAP
                		printf("se suma a las temperaturas el valor: %d\n",packed_buffer[packed_buffer_ptr]);
            		#endif
					
            		temp_avg[j] += packed_buffer[packed_buffer_ptr];	
            		packed_buffer_ptr++;
        	}

		/* Sumar los valores de las gpus */
        	/*for(int j = 0; j < n_gpus; j++){
            		packed_buffer_ptr++;
                    
            		#ifdef DEBUG_BIT_MAP
                		printf("se suma a memUsage de  gpus el valor: %d\n",packed_buffer[packed_buffer_ptr]);
            		#endif
			gpu_accu[j].memUsage += packed_buffer[packed_buffer_ptr];
			packed_buffer_ptr++;


			#ifdef DEBUG_BIT_MAP
                		printf("se suma a gpuUsage de  gpus el valor: %d\n",packed_buffer[packed_buffer_ptr]);
            		#endif	
			gpu_accu[j].gpuUsage += packed_buffer[packed_buffer_ptr];
			packed_buffer_ptr++;

			#ifdef DEBUG_BIT_MAP
                		printf("se suma a temperatura de gpus el valor: %d\n",packed_buffer[packed_buffer_ptr]);
            		#endif
			

			gpu_accu[j].temperature += packed_buffer[packed_buffer_ptr];
			packed_buffer_ptr++;


			#ifdef DEBUG_BIT_MAP
                		printf("se suma a powerUsage de gpus el valor: %d\n",packed_buffer[packed_buffer_ptr]);
            		#endif
				
			gpu_accu[j].powerUsage += packed_buffer[packed_buffer_ptr];
			packed_buffer_ptr++;
            		//temp_avg[j] += packed_buffer[packed_buffer_ptr];	
            		//packed_buffer_ptr++;
        	} */           	

    	}

    	/* Ahora se realiza la media por valor */
            
    	/* Media de la memoria */
    	memory_avg = memory_avg / n_samples;
    	codificar_cuartil(memory_avg);

    	#ifdef DEBUG_BIT_MAP
        	printf("media de la memoria es: %d\n",memory_avg);
    	#endif
            

    	/* Media de la cpu */
    	cpu_avg = cpu_avg / n_samples;
    	codificar_cuartil(cpu_avg);
            
    	#ifdef DEBUG_BIT_MAP
        	printf("media de la cpu es: %d\n",cpu_avg);
    	#endif

	/* Media de energia por cpu */
    	for(int i = 0; i < n_devices_io;i++){
        	pw_cpu[i] = pw_cpu[i] / n_samples;
        
        	#ifdef DEBUG_BIT_MAP
            		printf("media de las energias por cpu es: %d\n",pw_cpu[i]);
        	#endif
                
        	codificar_cuartil(pw_cpu[i]);
    	}


    	/* Media por dispositivo */
    	for(int i = 0; i < n_devices_io;i++){
        	dev_avg[i] = dev_avg[i] / n_samples;
        
        	#ifdef DEBUG_BIT_MAP
            		printf("media de los dispositivos es: %d\n",dev_avg[i]);
        	#endif
                
        	codificar_cuartil(dev_avg[i]);
    	}

    	/* Media por interfaz */
    	for(int i = 0; i < n_interfaces; i++){
        	net_avg[i] = net_avg[i] / n_samples;
        
        	#ifdef DEBUG_BIT_MAP
            		printf("media de las interfaces es: %d\n",net_avg[i]);
        	#endif

        	codificar_cuartil(net_avg[i]);
    	}

    	/* Media por temperatura */
    	for(int i = 0; i < n_core_temps; i++){
            	
        	temp_avg[i] = (unsigned char)temp_avg[i] / n_samples;
        
        	#ifdef DEBUG_BIT_MAP
            		printf("media de las temperaturas es: %d\n",temp_avg[i]);
        	#endif

        	codificar_cuartil(temp_avg[i]);
    	}

	/* GPUs average */
    	/*for(int i = 0; i < n_gpus; i++){
            	
        	gpu_accu[i].memUsage = (unsigned char)gpu_accu[i].memUsage / n_samples;
        	codificar_cuartil(gpu_accu[i].memUsage);
        	
		gpu_accu[i].gpuUsage = (unsigned char)gpu_accu[i].gpuUsage / n_samples;
        	codificar_cuartil(gpu_accu[i].gpuUsage);
        	
		gpu_accu[i].temperature = (unsigned char)gpu_accu[i].temperature / n_samples;
        	codificar_cuartil(gpu_accu[i].temperature);
        	
		gpu_accu[i].powerUsage = (unsigned char)gpu_accu[i].powerUsage / n_samples;
        	codificar_cuartil(gpu_accu[i].powerUsage);
        
        	#ifdef DEBUG_BIT_MAP
            		printf("media de la memUsage de las gpus es: %d\n",gpu_accu[i].memUsage);
            		printf("media de gpuUsage de las gpus es: %d\n",gpu_accu[i].gpuUsage);
            		printf("media de las temperaturas de las gpus es: %d\n",gpu_accu[i].temperature);
            		printf("media de powerUsage de las gpus es: %d\n",gpu_accu[i].powerUsage);
        	#endif


    	}*/

    /***
        Las posiciones del packed_buffer son:
        0-3: direccion ip
        4: memoria en GBytes
        5: numero de cpus
        6: numero de cores
        7: numero de dispositivos
        8: numero de interfaces
        9: numero de coretemps
        10: numero de muestras que se envian empaquetadas
        11: uso de la memoria en el tiempo de muestra
        12: cpu idle en el tiempo de muestra

        El resto de posiciones depende del numero de dispositivos, interfaces y coretemps
	1 bytes por numero de CPUs
        2 bytes por numero de dispositivos
        2 bytes por numero de interfaces
        2 bytes por numero de coretemps
	5 bytes por numero de GPUs
    ***/

}

void Packed_sample::codificar_cuartil(int cuartil){

    unsigned char * tmp_p = bit_map.get();
    if(cuartil < 25){
    
        tmp_p[byte_bit_map] = tmp_p[byte_bit_map] | (0 << 6 - bit_map_ptr);
        
    }else if(cuartil >= 25 && cuartil < 50){
        
        tmp_p[byte_bit_map] = tmp_p[byte_bit_map] | (1 << 6 - bit_map_ptr);
        
    }else if(cuartil >=50 && cuartil < 75){
    
        tmp_p[byte_bit_map] = tmp_p[byte_bit_map] | (2 << 6 - bit_map_ptr);
    }else{

        tmp_p[byte_bit_map] = tmp_p[byte_bit_map] | (3 << 6 - bit_map_ptr);
    
    }
	#ifdef DEBUG_LBBM
		//printf("byte es: %d bit es %d\n", byte_bit_map, bit_map_ptr);
    #endif


    bit_map_ptr += 2;

    if(bit_map_ptr > 6){
    
        byte_bit_map++;
        bit_map_ptr = 0;
    
    }

}
