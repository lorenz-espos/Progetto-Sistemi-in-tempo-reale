//Lorenzo Esposito N46004254
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <rtai_lxrt.h>
#include <rtai_mbx.h>
#include <rtai_shm.h>
#include <rtai_sem.h>
#include <rtai_msg.h>
#include <signal.h>
#include <sys/io.h>
#include "monitor.h"
#define CPUMAP 0x1 //per farlo eseguire su una cpu diversa rispetto a sensor

static RT_TASK *async_Task;
static RT_TASK *speedaper;
static RT_TASK *altitudeaper;
static RT_TASK *temperatureaper;
static RT_TASK *tbstask;
static MBX* mbx;
static MBX* mbxu;
static struct processed_sensors_data *process;
static struct raw_sensors_data *raw;
static pthread_t temp;
static pthread_t alt;
static pthread_t speed;
static pthread_t tbs;
static int start;
static int banda=2; //banda del TBS
RTIME deadline=0; //introduco una variabile deadline globale che userò per implementare il TBS
SEM* deadline_sem;

static void sig_handler(int signum){
	start = 0;
}

static void * altitudea(void * t){
	//il task alt dura il doppio rispetto ad altri task aperiodici TICK_TIME*2
		printf("avvio la creazione del task altitude..\n");
	if(!(altitudeaper = rt_task_init_schmod(nam2num("RTA101"), 0, STACK_SIZE, 0, SCHED_FIFO, CPUMAP))){
		printf("failed creating task\n");
		exit(1);
	}
	printf("task altitude creato\n");
	RTIME now;
	int msg;
	int i;
	int abuf[5];
	int cont=0;
	int aprocessato;
	rt_make_hard_real_time();//rendo il task hard real-time
	rt_printk("sono entrato nel task altitude hard \n");

	
	while(cont<21){
		rt_receive(tbstask,&msg);//recive bloccante perciò non va oltre finchè non arriva un messaggio da TBS attende
		now=rt_get_time();
		//sezione critica sulle deadline
		rt_sem_wait(deadline_sem);
		// formula del TBS d=max(rk,dk-1)+Ck/Us
		if(deadline<now){
			deadline=now+nano2count(TICK_TIME*2)*banda;
		}
		else{
			deadline=deadline+nano2count(TICK_TIME*2)*banda;
		}
		rt_sem_signal(deadline_sem);
		rt_task_set_resume_end_times(now,deadline);

		rt_sem_wait(raw->mutex);
		for(i=0;i<5;i++){
			abuf[i]=raw->altitudes[i];
		}
		rt_sem_signal(raw->mutex);

		rt_sem_wait(process->mutex2);
		aprocessato=process->altitude;
		rt_sem_signal(process->mutex2);

		rt_make_soft_real_time();//diventa soft realtime per effettuare le stampe

		for(i=0;i<5;i++){
			printf("altitude:  %d\n",abuf[i]);	
		}
		printf("altitude processato %d\n",aprocessato);
		cont++;

	}
	if(cont==20){
		msg=1;
		rt_mbx_send(mbxu,&msg,sizeof(int));//send bloccante
	}
	rt_task_delete(altitudeaper);
	return 0;

} 
static void * speeda(void * t){
	printf("avvio la creazione del task speed...\n");
		if(!(speedaper = rt_task_init_schmod(nam2num("RTA102"), 0, STACK_SIZE, 0, SCHED_FIFO, CPUMAP))){
		printf("failed creating speedtask\n");
		exit(1);
	}
	printf("task speed creato\n");
	int i;
	int cont=0;
	int msg;
	int sbuf[3];
	int sprocessato;
	RTIME now;
	rt_make_hard_real_time();
		rt_printk("sono entrato nel task speed hard \n");


	
	while(cont<11){
	rt_receive(tbstask,&msg);//recive bloccante aspetta finchè non arriva il messaggio da TBS
	now=rt_get_time();

		rt_sem_wait(deadline_sem);
		if(deadline<now){// formula del TBS d=max(rk,dk-1)+Ck/Us
			deadline=now+nano2count(TICK_TIME)*banda;
		}
		else{
			deadline=deadline+nano2count(TICK_TIME)*banda;
		}
		rt_sem_signal(deadline_sem);
	rt_task_set_resume_end_times(now,deadline);
	rt_sem_wait(raw->mutex);
	for(i=0;i<3;i++){
		sbuf[i]=raw->speeds[i];
	}
	rt_sem_signal(raw->mutex);

	rt_sem_wait(process->mutex2);
	sprocessato=process->speed;
	rt_sem_signal(process->mutex2);

	rt_make_soft_real_time();//diventa soft real time per effettuare le stampe

		for(i=0;i<3;i++){
			printf("speed %d\n",sbuf[i]);
	}
	printf("speed processato %d\n",sprocessato);
	cont++;
	}
	if(cont==10){
		msg=2;
		rt_mbx_send(mbxu,&msg,sizeof(int));
	}

	rt_task_delete(speedaper);
	return(0);
}
static void * temperaturea(void *t){
		printf("avvio la creazione del task temperature...\n");

		if(!(temperatureaper = rt_task_init_schmod(nam2num("RTA103"), 0, STACK_SIZE, 0, SCHED_FIFO, CPUMAP))){
		printf("failed creating temperaturetask \n");
		exit(1);
	}
	printf("task temperature creato\n");
		RTIME now;
		int cont=0;
		int i;
		int msg;
		int tbuf[3];
		int tprocessato;
	rt_make_hard_real_time();
		rt_printk("sono entrato nel task temperature hard \n");

	while(cont<11){
		rt_receive(tbstask,&msg);//recive bloccante aspetta finchè non arriva il messaggio da TBS
		now=rt_get_time();
		rt_sem_wait(deadline_sem);
	    if(deadline<now){// formula del TBS d=max(rk,dk-1)+Ck/Us
			deadline=now+nano2count(TICK_TIME)*banda;
		}
		else{
			deadline=deadline+nano2count(TICK_TIME)*banda;
		}
		rt_sem_signal(deadline_sem);
		rt_task_set_resume_end_times(now,deadline);

		rt_sem_wait(raw->mutex);
		for(i=0;i<3;i++){
			tbuf[i]=raw->temperatures[i];
		}
		rt_sem_signal(raw->mutex);

		rt_sem_wait(process->mutex2);
		tprocessato=process->temperature;
		rt_sem_signal(process->mutex2);

		rt_make_soft_real_time();//il task diventa soft real time per effettuare le stampe

		for(i=0;i<3;i++){
			printf("temperature %d\n",tbuf[i]);

		}
		printf("temperature processato %d\n",tprocessato);
		cont++;
	}
	if(cont==10){
		msg=3;
		rt_mbx_send(mbxu,&msg,sizeof(int));
	}
	
	rt_task_delete(temperatureaper);
    return(0);
    
}

static void * tbs_task(void *t){
	int msg;
	printf("avvio la creazione del task tbs...\n");
	if(!(tbstask = rt_task_init_schmod(nam2num("RTA104"), 0, STACK_SIZE, 0, SCHED_FIFO, CPUMAP))){
		printf("failed creating tbstask\n");
		exit(1);
	}
	printf("task tbs creato\n");
	rt_make_hard_real_time();
	rt_printk("sono entrato nel tbs hard \n");
	while(start){

		rt_mbx_receive(mbx,&msg,sizeof(int));
	if(msg==1){//in base al messaggio scelgo chi risvegliare
		//gestione temperature
		rt_send(temperatureaper,msg);
	
	
	}
	if(msg==2){//in base al messaggio scelgo chi risvegliare
		//gestione speed
		rt_send(speedaper,msg);
	}
	if(msg==3){//in base al messaggio scelgo chi risvegliare
		//getione altitude
		rt_send(altitudeaper,msg);
		

	}
	
	}
	rt_task_delete(tbstask);
    return(0);
}


int main(void) {
	//start=1; causa deadlock di tutto il SO
	signal(SIGINT,sig_handler);
	printf("il monitor è stato avviato inizializzo il task main...\n");
	if(!(async_Task = rt_task_init_schmod(nam2num("RTA105"), 0, STACK_SIZE, 0, SCHED_FIFO, CPUMAP))){
		printf("failed creating maintask\n");
		exit(1);
	}
	printf("il task main è stato creato\n");
	rt_make_hard_real_time();
	//faccio diventare il task hard real time
	process=rtai_kmalloc(PROC_SEN_SHM,sizeof(struct processed_sensors_data));
	raw=rtai_kmalloc( RAW_SEN_SHM,sizeof(struct raw_sensors_data));
	raw->mutex=rt_typed_named_sem_init("M1",1,BIN_SEM | PRIO_Q);  
	process->mutex2=rt_typed_named_sem_init("M2",1,BIN_SEM | PRIO_Q);
	deadline_sem=rt_typed_named_sem_init("M3",1,BIN_SEM | PRIO_Q);
	mbx = rt_typed_named_mbx_init(MAILBOX_ID,MAILBOX_SIZE,FIFO_Q);
	mbxu = rt_typed_named_mbx_init(MAILBOX_ID2,MAILBOX_SIZE,FIFO_Q);
		//creazione dei thread
		pthread_create(&tbs, NULL,tbs_task, NULL);
		pthread_create(&temp, NULL,temperaturea, NULL);
		pthread_create(&speed, NULL,speeda, NULL);
		pthread_create(&alt, NULL,altitudea, NULL);

	while(start){
		rt_sleep(10000000);//aggiungo una sleep e un while start nel main
	}



	rtai_kfree(RAW_SEN_SHM);//libero la shared memory
  	rtai_kfree(PROC_SEN_SHM);//libero la shared memory
	rt_named_mbx_delete(mbx);//cancella la mailbox
	rt_named_mbx_delete(mbxu);//cancella la mailbox2
    rt_task_delete(async_Task);//cancello il task

	return 0;
}