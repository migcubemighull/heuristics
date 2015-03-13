#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "msg/msg.h"            /* Yeah! If you want to use msg, you need to include msg/msg.h */
#include "xbt/sysdep.h"         /* calloc, printf */

/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
#include "xbt/asserts.h"

///////////////////////////////////////////////////////////////////////////////
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,"Messages specific for this msg example");
#define task_comp_size 900000000
#define task_comm_size 1000000
#define SUPERSTEPS 80 // Number of supersteps
#define PROCESSES 60 // Total of processes, based on deployment files
#define ALPHA 8 // alpg
#define LOWEST_ALPHA 8
#define DELTA 0.2
#define COMPUTATION_COMMUNICATION_PATTERN 0.3
#define SELECTION_HEURISTIC_ONE 1
#define SELECTION_HEURISTIC_PERCENTAGE 2
#define SELECTION_HEURISTIC_MIGCUBE 3
#define SELECTION_HEURISTIC_MIGHULL 4
#define SELECTION_HEURISTIC_PERCENTAGE_VALUE 0.2
#define SELECTION_HEURISTIC SELECTION_HEURISTIC_MIGCUBE
#define MAXIMO_SUPERSTEPS_SEM_MIGRAR 4

#define SUPERSTEPS2 100
#define ELEMENTS_IN_A_LINE 40
#define LINES_PER_PROCESSES 40
#define LINES LINES_PER_PROCESSES * PROCESSES
#define COLUMNS ELEMENTS_IN_A_LINE * PROCESSES
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
typedef struct
{
	unsigned int id, id_manager, alpha, alpha_counter;
	double *vector_superstep_time, *instructions, computation_pattern, *computation_time, ctp;	
	double **communication_time, **communication_amount, *communication_pattern, *btp;
	double *computation_metric, *communication_metric, *memory_metric, *pm;
}BSP_process_data_t;

typedef struct
{
	double value;
	int set;
	int process;
	int migrate;
	int superstep;
	
	//parameters for new heuristics
	double x, y, z;
}PM_t;

typedef struct
{
	double *latency;
	int adaptacao;
}Manager_data_t;

typedef struct
{
	int id, x_y, x_z, y_z;
}MigHull_migrate;

typedef struct
{
	int id;
	double point_a, point_b;
}MigHull_sides_t;

typedef double ret_desvios[3];

PM_t list[PROCESSES];
BSP_process_data_t data[PROCESSES];
Manager_data_t data_manager[PROCESSES];

MigHull_migrate result_hull[PROCESSES];
MigHull_sides_t lado_xy[PROCESSES], lado_xz[PROCESSES], lado_yz[PROCESSES];
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
int verify_set(char *mailbox_send, int pid)
{
	msg_host_t host;
	char pid_char[8], host_comparacao[256], prefix[256];
	int pid_real, sets, i=0, base;
	pid_real = pid;
	sets = atoi(MSG_process_get_property_value(MSG_process_from_PID(PROCESSES + 1), "amount"));
	base = PROCESSES+1;
	if (mailbox_send !=NULL)
	{
		sscanf(mailbox_send, "process-%s",pid_char);
		pid_real = atoi(pid_char);
	}
	host = MSG_process_get_host(MSG_process_from_PID(pid_real));
	//XBT_INFO("1-Antes %s e %s",host_comparacao, MSG_host_get_name(host));
	strcpy(host_comparacao, MSG_host_get_name(host));
	//XBT_INFO("1-Depois %s e %s",host_comparacao, MSG_host_get_name(host));
	for(i=0; ;i++)
	{
		if (host_comparacao[i] == '-' )
		{
			host_comparacao[i+1] = '\0';
			break;
		}
	}
	
	for(i=PROCESSES+1; i<base+sets; i++)
	{
		//XBT_INFO("2-Antes %s e %s",prefix, MSG_process_get_property_value(MSG_process_from_PID(i), "prefix"));
		strcpy(prefix, MSG_process_get_property_value(MSG_process_from_PID(i), "prefix"));
		//XBT_INFO("2-Depois %s e %s",prefix, MSG_process_get_property_value(MSG_process_from_PID(i), "prefix"));
		if (strcmp(prefix, host_comparacao)==0)
		{
			i = i - PROCESSES - 1;
			break;
		}
	}
	return i;
}
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
int number_sets()
{
	int sets;
	sets = atoi(MSG_process_get_property_value(MSG_process_from_PID(PROCESSES + 1), "amount"));
	return sets;	
}
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
double aging(double *vetor, int elementos)
{
	double resultado=0, peso=0;
	int i;
	for (i=elementos-1, peso=0.5; i>=0; i--)
	{
		resultado += vetor[i] * peso;
		peso = (double) peso/2; 
	}
	return resultado;
}
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
double calculate_pattern(double *vetor, int elementos)
{
	int i;
	double result=1;
	for(i=elementos -1; i >=0; i--)
	{
		if (aging(vetor, elementos) >= vetor[i] * (1.0-COMPUTATION_COMMUNICATION_PATTERN) && aging(vetor, elementos) <= vetor[i] * (1.0+COMPUTATION_COMMUNICATION_PATTERN))
		{
			result += (double)1/(double)elementos;
		}
		else
		{
			result -= (double)1/(double)elementos;
		}
	}
	if (result >= 1)
	{
		result = 1;
	}
	else if (result < 0)
	{
		result = 0.0005;
	}
	return result;
}
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
int number_process(char *machine)
{
	int retorno=0;
	char comparison[100];
	msg_host_t host;
	int i;
	for (i=1; i<=PROCESSES; i++)
	{
		host = MSG_process_get_host(MSG_process_from_PID(i));
		//XBT_INFO("3-Antes %s e %s",comparison, (char *) MSG_host_get_name(host));
		strcpy(comparison, MSG_host_get_name(host)); //(char *) 
		//XBT_INFO("3-Depois %s e %s",comparison, (char *) MSG_host_get_name(host));
		if (strcmp(comparison, machine) == 0)
		{
			retorno++;
		}
	}
	return retorno;
}
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
double iset(int target_set)
{
	int i,j, sets, base, radical;
	double *retorno, *capacidade_set, greatest=0, focus_retorno;
	char machine[100], sufix[256], prefix[256], power[256];
	sets = atoi(MSG_process_get_property_value(MSG_process_from_PID(PROCESSES + 1), "amount"));
	base = atoi(MSG_process_get_property_value(MSG_process_from_PID(PROCESSES + 1), "base"));
	capacidade_set = (double *) malloc(sizeof(double) * sets );
	retorno = (double *) malloc(sizeof(double) * sets);
	for (i=0; i<sets; i++)
	{
		radical = atoi(MSG_process_get_property_value(MSG_process_from_PID(base+i), "radical"));
		//XBT_INFO("4-Antes %s e %s",sufix,MSG_process_get_property_value(MSG_process_from_PID(base+i), "sufix"));
		strcpy(sufix,MSG_process_get_property_value(MSG_process_from_PID(base+i), "sufix"));
		//XBT_INFO("4-Depois %s e %s",sufix,MSG_process_get_property_value(MSG_process_from_PID(base+i), "sufix"));
		//XBT_INFO("5-Antes %s e %s", prefix,MSG_process_get_property_value(MSG_process_from_PID(base+i), "prefix"));
		strcpy(prefix,MSG_process_get_property_value(MSG_process_from_PID(base+i), "prefix"));
		//XBT_INFO("5-Despois %s e %s", prefix,MSG_process_get_property_value(MSG_process_from_PID(base+i), "prefix"));
		//XBT_INFO("6-Antes %s e %s", power, MSG_process_get_property_value(MSG_process_from_PID(base+i), "power"));
		strcpy(power, MSG_process_get_property_value(MSG_process_from_PID(base+i), "power"));
		//XBT_INFO("6-Depois %s e %s", power, MSG_process_get_property_value(MSG_process_from_PID(base+i), "power"));
		capacidade_set[i] = 0.0;
		for(j=1; j<=radical; j++)
		{
			sprintf(machine, "%s%d%s", prefix, j, sufix);
			capacidade_set[i] += atof(power) / (number_process(machine) +1);
		}
	}
	for (i=0; i<sets; i++)
	{
		if ( capacidade_set[i] > greatest)
		{
			greatest = capacidade_set[i];
		}
	}
	for (i=0; i<sets; i++)
	{
		retorno[i] = capacidade_set[i] / greatest;
	}
	focus_retorno = retorno[target_set];
	free(retorno);
	free(capacidade_set);
	return focus_retorno;
}
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
void clean_process_data(int i)
{
	int j, k;
	data[i].alpha_counter=0;
	data[i]. ctp = 0;
	data[i].computation_pattern = 1;
	free(data[i].vector_superstep_time);
	free(data[i].instructions);
	free(data[i].computation_time);
	for (j=0; j< number_sets();j++)
	{
		data[i].btp[j] = 0;
		data[i].communication_pattern[j] = 1;
		data[i].communication_metric[j] = 0;
		data[i].computation_metric[j] = 0;
		data[i].memory_metric[j] = 0;
		data[i].pm[j] = 0;
		free(data[i].communication_time[j]);
		free(data[i].communication_amount[j]);
		data[i].communication_time[j] = (double *)malloc(data[i].alpha * sizeof(double));
		data[i].communication_amount[j] = (double *)malloc(data[i].alpha * sizeof(double));
		for(k=0; k< data[i].alpha; k++)
		{
			data[i].communication_time[j][k] = 0;
			data[i].communication_amount[j][k] = 0;
		}
	}
	data[i].vector_superstep_time = (double *)malloc(data[i].alpha * sizeof(double));
	data[i].instructions = (double *)malloc(data[i].alpha * sizeof(double));
	data[i].computation_time = (double *)malloc(data[i].alpha * sizeof(double));
	for (k=0; k < data[i].alpha; k++)
	{
		data[i].vector_superstep_time[k] = 0;
		data[i].instructions[k] = 0;
		data[i].computation_time[k] = 0;
	}
}
///////////////////////////////////////////////////////////////////////////////
		
///////////////////////////////////////////////////////////////////////////////
PM_t *get_largest(double *vector, int count, int process, int superstep, double *vec_comp, double *vec_comm, double *vec_memm)
{
	PM_t *result = NULL;
	free(result);
	int i;
	double greatest = 0;
	result = (PM_t *) malloc(sizeof(PM_t));

	result->x = 0;
	result->y = 0;
	result->z = 0;

	for(i=0; i< count; i++)
	{
		if (vector[i] > greatest)
		{
			result->value = vector[i];
			result->set = i;
			result->migrate = 0;
			result->process = process;
			result->superstep = superstep;
			greatest = vector[i];
		}
		
		if(SELECTION_HEURISTIC == SELECTION_HEURISTIC_MIGCUBE){
			result->x = vec_comp[i];
			result->y = vec_comm[i];
			result->z = vec_memm[i];
		}
		else
		{
			if(vec_comp[i] > result->x){
				result->x = vec_comp[i];
			}
			if(vec_comm[i] > result->y){
				result->y = vec_comm[i];
			}
			if(vec_memm[i] > result->z){
				result->z = vec_memm[i];
			}	
		}

	}
	return result;
}
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
int rescheduling(double superstep_time, double computation_time, int computation_instructions, double *communication_time, int *communication_amount, int superstep)
{
	PM_t *largest;
	msg_task_t task_send = NULL, task_recv = NULL;
	char mailbox_send[256], mailbox_recv[256];
	int i,j, sets;
	//XBT_INFO("Entrou no rescheduling, comp time=%f, instructions=%d, superstep_time=%f", computation_time, computation_instructions,superstep_time);
	sets = atoi(MSG_process_get_property_value(MSG_process_from_PID(PROCESSES + 1), "amount"));
	i = MSG_process_self_PID() - 1;
	data[i].vector_superstep_time[data[i].alpha_counter] = superstep_time;
	data[i].computation_time[data[i].alpha_counter] = computation_time;
	data[i].instructions[data[i].alpha_counter] = computation_instructions;
	for (j=0; j< sets; j++)
	{
		//XBT_INFO("time=%f e amm=%d", communication_time[j],communication_amount[j]);
		data[i].communication_time[j][data[i].alpha_counter] = communication_time[j];
		data[i].communication_amount[j][data[i].alpha_counter] = communication_amount[j];
	}
	data[i].alpha_counter++;
	if (data[i].alpha_counter == data[i].alpha)
	{
		//XBT_INFO("Comecou a parte avancada do Rescheduling, superstep=%d", superstep);
		data[i].computation_pattern = calculate_pattern(data[i].instructions, data[i].alpha);
		data[i].ctp = aging(data[i].computation_time, data[i].alpha);
		for(j=0;j<sets;j++)
		{
			data[i].communication_pattern[j] = calculate_pattern(data[i].communication_amount[j], data[i].alpha);
			data[i].btp[j] = aging(data[i].communication_time[j], data[i].alpha);
			data[i].computation_metric[j] = data[i].computation_pattern * data[i].ctp * iset(j);
			//XBT_INFO("O iset de %d eh %f", j, iset(j));
			data[i].communication_metric[j] = data[i].communication_pattern[j] * data[i].btp[j];
			data[i].memory_metric[j] = data_manager[verify_set(NULL,MSG_process_self_PID())].latency[j] * aging(data[i].communication_amount[j], data[i].alpha);
			data[i].pm[j] = data[i].computation_metric[j] + data[i].communication_metric[j] - data[i].memory_metric[j];
			//XBT_INFO("Meu PM para o conjunto %d eh %f[comp=%f][comm=%f][mem=%f]. Meu i=%d", j, data[i].pm[j], data[i].computation_metric[j], data[i].communication_metric[j], data[i].memory_metric[j], i);
		}
		largest = get_largest(data[i].pm, sets, i, superstep, data[i].computation_metric, data[i].communication_metric, data[i].memory_metric);
		//largest = get_largest(data[i].pm, sets, i, superstep);
		task_send = MSG_task_create("sending", 0, 30, largest);
		sprintf(mailbox_send, "process-manager-%d", data[i].id_manager);
		MSG_task_send(task_send, mailbox_send);
		
		sprintf(mailbox_recv, "process-manager-%d", MSG_process_self_PID());
		MSG_task_recv(&(task_recv), mailbox_recv);
		//data[i].alpha= (int) MSG_task_get_compute_duration(task_recv);
		//MSG_task_destroy(task_recv);
		if (MSG_process_self_PID() == 1)
		{
			XBT_INFO("** Novo alpha eh %d", data[i].alpha); 
		}
		task_recv = NULL;
		clean_process_data(i);

//		for (j=0; j< PROCESSES; j++)
//		{
//			XBT_INFO("Dados dos processos: %d, manager=%d, alpha=%d", data[j].id, data[j].id_manager, data[j].alpha);
//		}
	}
	return 0;
}
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
void barrier(double superstep_time, double computation_time, int computation_instructions, double *communication_time, int *communication_amount, int superstep)
{
	if (MSG_process_self_PID() == 1)
	{
		int i=0;
		char mailbox_send[20];
		msg_task_t task_barrier[PROCESSES-1];
		msg_comm_t async_send[PROCESSES-1];
		for(i=2; i <= PROCESSES; i++)
		{
			task_barrier[i-2] = MSG_task_create("barrier" , 0, 10, NULL);
			sprintf(mailbox_send, "barrier-%d", i);
			async_send[i-2] = MSG_task_isend(task_barrier[i-2], mailbox_send);
		}
		for(i=2; i <= PROCESSES; i++)
		{
			MSG_comm_wait(async_send[i-2], -1);
		}
	}
	else
	{
		char mailbox_recv[20];
		msg_task_t task_barrier = NULL;
		msg_comm_t async_recv;
		sprintf(mailbox_recv, "barrier-%d", MSG_process_self_PID());
		async_recv = MSG_task_irecv(&(task_barrier), mailbox_recv);
		MSG_comm_wait(async_recv, -1);
	}
#if defined (S2) || defined (S3)
	rescheduling(superstep_time, computation_time, computation_instructions, communication_time, communication_amount, superstep);
#endif
}
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
void init_migbsp()
{
	int i, j, k, sets, base;
	i = MSG_process_self_PID() - 1;
	data[i].vector_superstep_time = (double *)malloc(ALPHA * sizeof(double));
	data[i].instructions = (double *)malloc(ALPHA * sizeof(double));
	data[i].computation_time = (double *)malloc(ALPHA * sizeof(double));
	data[i].computation_pattern = 1;
	data[i]. ctp = 0;
	data[i].id_manager = atoi(MSG_process_get_property_value(MSG_process_self(),"manager"));
	data[i].id = MSG_process_self_PID();
	data[i].alpha = ALPHA;
	data[i].alpha_counter = 0;
	sets = atoi(MSG_process_get_property_value(MSG_process_from_PID(PROCESSES + 1), "amount"));
	data[i].computation_metric = (double *)malloc(sets * sizeof(double));
	data[i].communication_metric = (double *)malloc(sets * sizeof(double));
	data[i].memory_metric = (double *)malloc(sets * sizeof(double));
	data[i].pm = (double *)malloc(sets * sizeof(double));
	data[i].communication_pattern = (double *)malloc(sets * sizeof(double));
	data[i].btp = (double *)malloc(sets * sizeof(double));
	data[i].communication_time = (double **)malloc(sets * sizeof(double *));
	data[i].communication_amount = (double **)malloc(sets * sizeof(double *));
	for (j=0; j< sets; j++)
	{
		data[i].communication_time[j] = (double *)malloc(ALPHA * sizeof(double));
		data[i].communication_amount[j] = (double *)malloc(ALPHA * sizeof(double));
		for (k=0; k< ALPHA; k++)
		{
			data[i].communication_time[j][k] = 0;
			data[i].communication_amount[j][k] = 0;
		}
		data[i].communication_pattern[j] = 1;
		data[i].btp[j] = 0;
	}
	if (MSG_process_get_PID(MSG_process_self()) == 1)
	{
		base = atoi(MSG_process_get_property_value(MSG_process_from_PID(PROCESSES + 1), "base"));
		for(i=0; i<sets; i++)
		{
			MSG_process_resume(MSG_process_from_PID(base + i));
		}
	}
}
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
void finalize_migbsp()
{
	int i, sets, base;
	if (MSG_process_get_PID(MSG_process_self()) == 1)
	{
		sets = atoi(MSG_process_get_property_value(MSG_process_from_PID(PROCESSES + 1), "amount"));
		base = atoi(MSG_process_get_property_value(MSG_process_from_PID(PROCESSES + 1), "base"));
		for(i=0; i<sets; i++)
		{
			MSG_process_kill(MSG_process_from_PID(base + i));
		}
	}
}
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
void select_migration(int pid)
{
	int i;
	for (i=0; i< PROCESSES; i++)
	{
		if (list[i].process == pid)
		{
			list[i].migrate = 1;
		}
	}
}
///////////////////////////////////////////////////////////////////////////////

//Start Cube calculate	
///////////////////////////////////////////////////////////////////////////////
void cube_calculate(PM_t *lista)
{

	int i;
	double soma_total=0, soma_dist, soma_x, soma_y, soma_z;
	double min_x, max_x, min_y, max_y, min_z, max_z;
	//int count=0;

	//XBT_INFO("Calculate MigCube!!");

	//Imprimi valores Iniciais
	//for(i=0; i< PROCESSES; i++)
	//	{
	//		XBT_INFO("ID: %d em %d, PM: %f, X: %f, Y: %f e Z: %f", lista[i].process, lista[i].set,lista[i].value,lista[i].x, lista[i].y, lista[i].z);	
	//	}
	
	//Caclula a distância de cada processo
	//Inicia do segundo maior PM até o ultimo
	for(i=1; i< PROCESSES; i++)
		{
			soma_dist=0;

			soma_x = pow((lista[i].x-lista[0].x),2);
			soma_y = pow((lista[i].y-lista[0].y),2);
			soma_z = pow((lista[i].z-lista[0].z),2);
			//XBT_INFO("PID=%d, X = %f, Y = %f, Z = %f", lista[i].process, soma_x, soma_y, soma_z);	
			
			soma_dist = soma_x + soma_y + soma_z;
			//XBT_INFO("Soma : %f", soma_dist);

			soma_total = soma_total + sqrt(soma_dist);
			//XBT_INFO("Raiz: %f", soma_total);

		}

	//Cacula a media
	soma_total = soma_total/(PROCESSES-1);
	//XBT_INFO("Media: %f", soma_total);

	//Calculando os centros das laterais do cubo
	min_x = lista[0].x - soma_total;
	max_x = lista[0].x + soma_total;
	min_y = lista[0].y - soma_total;
	max_y = lista[0].y + soma_total;
	min_z = lista[0].z - soma_total;
	max_z = lista[0].z + soma_total;

	//XBT_INFO("MinX=%f,MaxX=%f,MinY=%f,MaxY=%f,MinZ=%f,MaxZ=%f",min_x,max_x,min_y,max_y,min_z,max_z);
	//Verifica quais pontos estao dentro da area do cubo
	for(i=0; i< PROCESSES; i++)
		{
			
			//XBT_INFO("Ponto %d x=%f, y=%f, z=%f",lista[i].process,lista[i].x,lista[i].y,lista[i].z);
			if(
				lista[i].x >= min_x &&
				lista[i].x <= max_x &&
				lista[i].y >= min_y &&
				lista[i].y <= max_y &&
				lista[i].z >= min_z &&
				lista[i].z <= max_z 
				//&& count <2
				)
			{
				//count++;
				//XBT_INFO("Entrou ID: %d",lista[i].process);
				select_migration(lista[i].process);
			}

		}
	
}
///////////////////////////////////////////////////////////////////////////////
//End Cube Calculate
//Start Hull calculate
///////////////////////////////////////////////////////////////////////////////
void setMigrateLado(int id, int lado)
{

	if(lado == 1)
	{
		result_hull[id].x_y = 1;
	}
	else
	{
		if(lado == 2)
		{
			result_hull[id].x_z = 1;
		}
		else //lado 3
		{
			result_hull[id].y_z = 1;
		}
	}
	//XBT_INFO("Lado %d, id %d e lista id %d", lado, id, result_hull[id].id);

}
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
double getPointDistance(double inicio_a, double inicio_b, double fim_a, double fim_b, double ponto_a, double ponto_b)
{

	double distancia, saidaA, saidaB;
    double vLineA, vLineB, vPointA, vPointB;

    vLineA = fim_a - inicio_a;
    vLineB = fim_b - inicio_b;
    vPointA = ponto_a - inicio_a;
    vPointB = ponto_b - inicio_b;

    saidaA = vPointB * vLineA;
    saidaB = vPointA * vLineB;

    distancia = saidaA - saidaB;

    return distancia;

}
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
void getConvexHull(MigHull_sides_t *temp, int plano, double stdv)
{
	
	//Funcao que calcula quais pontos estao dentro da convex hull
	int i, menor_x, maior_x;
	double distancia;

	//verifica a ordem do maior e menor x, conforme PM
	// Salva na ordem, menor 0 e mairo 1
	menor_x = temp[0].id;
	maior_x = temp[1].id;
	//Verica se for o inverso
	if(temp[0].point_a > temp[1].point_a)
	{
		//se o X do ponto 0 for maior do que o 1, inverte
		menor_x = temp[1].id;
		maior_x = temp[0].id;

	}

	//Busca a distancia de cada ponto na linha
	for(i=0; i< PROCESSES; i++)
		{
			//XBT_INFO("Id %d, A %f e B %f, plano %d e desvio %f, distancia %f", temp[i].id,temp[i].point_a, temp[i].point_b, plano, stdv, distancia);
			distancia = 0;

			distancia = getPointDistance(temp[menor_x].point_a, temp[menor_x].point_b, temp[maior_x].point_a,temp[maior_x].point_b, temp[i].point_a, temp[i].point_b);

			if(distancia >= 0 && distancia < stdv)
			{
				//Chama uma funcao, que verifca qual lado e seta em result_hull para migrar
				setMigrateLado(i, plano);
			}
			//XBT_INFO("Primeiro Id %d, A %f e B %f, plano %d e desvio %f, distancia %f", temp[i].id,temp[i].point_a, temp[i].point_b, plano, stdv, distancia);
			//Execute inverendo o X 
			distancia = 0;

			distancia = getPointDistance(temp[maior_x].point_a, temp[maior_x].point_b, temp[menor_x].point_a, temp[menor_x].point_b, temp[i].point_a, temp[i].point_b);

			if(distancia >= 0 && distancia < stdv)
			{
				//Chama uma funcao, que verifca qual lado e seta em result_hull para migrar
				setMigrateLado(i, plano);
			}
			//XBT_INFO("Segundo Id %d, A %f e B %f, plano %d e desvio %f, distancia %f", temp[i].id,temp[i].point_a, temp[i].point_b, plano, stdv, distancia);

		}
}
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
double* getStdDev(PM_t *temp)
{

	//Funcao para retornar o desvio padrao dos valores de cada eixo de dados

	static double retorno[3];
	double aux, desvio_x=0, desvio_y=0, desvio_z=0;
	double soma_x=0, soma_y=0, soma_z=0, media_x=0, media_y=0, media_z=0;
	int i;

	//soma a media
	for(i=0; i< PROCESSES; i++)
		{
			soma_x = soma_x + temp[i].x;
			soma_y = soma_y + temp[i].y;
			soma_z = soma_z + temp[i].z;
			//XBT_INFO("Valores %f %f %f", temp[i].x, temp[i].y, temp[i].z);
		}

	media_x = soma_x / PROCESSES;
	media_y = soma_y / PROCESSES;
	media_z = soma_z / PROCESSES;
	
	soma_x=0, soma_y=0, soma_z=0;
	//XBT_INFO("Medias %f %f %f", media_x, media_y, media_z);
	
	//calcula o desvio
	for(i=0; i< PROCESSES; i++)
		{
			aux = 0;
			aux = temp[i].x - media_x;
			soma_x = soma_x + (pow(aux,2));

			aux = 0;
			aux = temp[i].y - media_y;
			soma_y = soma_y + (pow(aux,2));

			aux = 0;
			aux = temp[i].z - media_z;
			soma_z = soma_z + (pow(aux,2));	
		}

	desvio_x = soma_x/PROCESSES;
	desvio_y = soma_y/PROCESSES;
	desvio_z = soma_z/PROCESSES;
	//XBT_INFO("Desvio %f %f %f", desvio_x, desvio_y, desvio_z);
	
	retorno[0]=sqrt(desvio_x);
	retorno[1]=sqrt(desvio_y);
	retorno[2]=sqrt(desvio_z);

	return retorno;

}
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
void hull_calculate(PM_t *lista)
{
	
	//XBT_INFO("Calculate MigHull!!");

	int i;
	double *ret_stdv, maior_stdv_xy, maior_stdv_xz, maior_stdv_yz;
	//int count=0;
	
	//Busca o desvio padrao em cada eixo
	ret_stdv = getStdDev(lista);

	//XBT_INFO("Desvio %f %f %f", ret_stdv[0], ret_stdv[1], ret_stdv[2]);

	//Inicializa result_hull
	//Inicializa o vetor que valida se o processo ID migra naquele lado
	//Divide pontos em 3 blocos
	for(i=0; i< PROCESSES; i++)
		{
			result_hull[i].id = lista[i].process;
			result_hull[i].x_y = 0;
			result_hull[i].x_z = 0;
			result_hull[i].y_z = 0;

			lado_xy[i].id = lista[i].process;
			lado_xy[i].point_a = lista[i].x;
			lado_xy[i].point_b = lista[i].y;

			lado_xz[i].id = lista[i].process;
			lado_xz[i].point_a = lista[i].x;
			lado_xz[i].point_b = lista[i].z;

			lado_yz[i].id = lista[i].process;
			lado_yz[i].point_a = lista[i].y;
			lado_yz[i].point_b = lista[i].z;
		}

	//Verificar qual desvio padrao utilizar
	//Neste caso estou pegando o maior dos dois
	maior_stdv_xy = ret_stdv[0];
	maior_stdv_xz = ret_stdv[0];
	maior_stdv_yz = ret_stdv[1];
	
	if(ret_stdv[1] > ret_stdv[0])
	{//Inverte X e Y
		maior_stdv_xy = ret_stdv[1];
	}
	if(ret_stdv[2] > ret_stdv[0])
	{//Inverte X e Z
		maior_stdv_xy = ret_stdv[2];
	}
	if(ret_stdv[2] > ret_stdv[1])
	{//Inverte Y e Z
		maior_stdv_xy = ret_stdv[2];
	}

	// Calcula X e Y
	getConvexHull(lado_xy, 1, maior_stdv_xy);
	
	// Calcula X e Z
	getConvexHull(lado_xz, 2, maior_stdv_xz);
	
	// Calcula Y e Z
	getConvexHull(lado_yz, 3, maior_stdv_yz);

	//Verifica quais pontos estao nas 3 listas

	for(i=0; i< PROCESSES; i++)
		{
			//Se ele tem marcado 1 nos tres planos, migra
			if(result_hull[i].x_y == 1 && result_hull[i].x_z == 1 && result_hull[i].y_z == 1 /*&& count <4*/)
			{
				//count++;
				//XBT_INFO("Entrou para ID: %d",i);
				select_migration(lista[i].process);
			}
		}
	

}
///////////////////////////////////////////////////////////////////////////////
//End Hull Calculate
///////////////////////////////////////////////////////////////////////////////
int migration_heuristic(PM_t *ordered)
{
	int i;
	if (SELECTION_HEURISTIC == SELECTION_HEURISTIC_ONE)
	{
		select_migration(ordered[0].process);
	}
	else if (SELECTION_HEURISTIC == SELECTION_HEURISTIC_PERCENTAGE)
	{
		for(i=0; i< PROCESSES; i++)
		{
			if (ordered[i].value > ordered[0].value * SELECTION_HEURISTIC_PERCENTAGE_VALUE)
			{
				select_migration(ordered[i].process);
			}
		}
	}
	else if (SELECTION_HEURISTIC == SELECTION_HEURISTIC_MIGCUBE)
	{
		cube_calculate(ordered);
	}
	else if (SELECTION_HEURISTIC == SELECTION_HEURISTIC_MIGHULL)
	{
		hull_calculate(ordered);
	}

	return 0;
}
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
void verify_migration()
{
	PM_t temp[PROCESSES], aux;
	int i,j;
	for(i=0; i< PROCESSES; i++)
	{
		temp[i].set = list[i].set;	
		temp[i].process = list[i].process;	
		temp[i].migrate = list[i].migrate;	
		temp[i].value = list[i].value;	
		temp[i].x = list[i].x;	
		temp[i].y = list[i].y;	
		temp[i].z = list[i].z;	
	}
	for(i=0; i< PROCESSES; i++)
	{
		for(j=0; j<PROCESSES; j++)
		{
			if (temp[i].value > temp[j].value)
			{
				aux.set = temp[j].set;
				aux.process = temp[j].process;
				aux.migrate = temp[j].migrate;
				aux.value = temp[j].value;
				aux.x = temp[j].x;
				aux.y = temp[j].y;
				aux.z = temp[j].z;

				temp[j].set = temp[i].set;
				temp[j].process = temp[i].process;
				temp[j].migrate = temp[i].migrate;
				temp[j].value = temp[i].value;
				temp[j].x = temp[i].x;
				temp[j].y = temp[i].y;
				temp[j].z = temp[i].z;
				
				temp[i].set = aux.set;
				temp[i].process = aux.process;
				temp[i].migrate = aux.migrate;
				temp[i].value = aux.value;
				temp[i].x = aux.x;
				temp[i].y = aux.y;
				temp[i].z = aux.z;
			}
		}
	}
	//for(i=0; i< PROCESSES; i++)
	//{
//		XBT_INFO("List, processo=%d, value=%f, set destino=%d, migrate=%d", temp[i].process, temp[i].value, temp[i].set, temp[i].migrate);
//	}
	migration_heuristic(temp); 
}
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
char *selecting_destination(int *vec)
{
	int i, radical, menor=10000, quantos;
	char sufix[100], prefix[100], machine[256], *retorno=NULL;
	radical = atoi(MSG_process_get_property_value(MSG_process_from_PID(MSG_process_self_PID()), "radical"));
	//XBT_INFO("7-Antes %s e %s", sufix,MSG_process_get_property_value(MSG_process_from_PID(MSG_process_self_PID()), "sufix"));
	strcpy(sufix,MSG_process_get_property_value(MSG_process_from_PID(MSG_process_self_PID()), "sufix"));
	//XBT_INFO("7-Depois %s e %s", sufix,MSG_process_get_property_value(MSG_process_from_PID(MSG_process_self_PID()), "sufix"));
	//XBT_INFO("8-Antes %s e %s", prefix,MSG_process_get_property_value(MSG_process_from_PID(MSG_process_self_PID()), "prefix"));
	strcpy(prefix,MSG_process_get_property_value(MSG_process_from_PID(MSG_process_self_PID()), "prefix"));
	//XBT_INFO("8-Depois %s e %s", prefix,MSG_process_get_property_value(MSG_process_from_PID(MSG_process_self_PID()), "prefix"));
	for (i=1; i<= radical; i++)
	{
		sprintf(machine, "%s%d%s", prefix, i, sufix);
		quantos = number_process(machine);
		if (quantos + vec[i-1] < menor)
		{
			menor = quantos + vec[i-1];
		}
	}
	for (i=1; i<= radical; i++)
	{
		sprintf(machine, "%s%d%s", prefix, i, sufix);
		quantos = number_process(machine);
		if (quantos + vec[i-1] == menor)
		{
			vec[i-1] = vec[i-1] + 1;
			retorno = malloc(256);
			//XBT_INFO("9-Antes %s e %s", retorno, machine);
			strcpy(retorno, machine);
			//XBT_INFO("9-Depois %s e %s", retorno, machine);
			break;
		}
	}
	//for(i=0; i< radical; i++)
	//{
	//	XBT_INFO("vec[%d]=%d", i, vec[i]);
	//}
	return retorno;
}
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
int perform_migration(int superstep)
{
	int retorno = 0, i, id, radical=1, *vec,contador=0;
	char *target;
	id = atoi(MSG_process_get_property_value(MSG_process_self(), "set"));
	radical = atoi(MSG_process_get_property_value(MSG_process_self(), "radical"));
	//XBT_INFO("Teste %d",radical);
	vec = (int *) malloc (sizeof(int) * radical);
	for (i=0; i< radical; i++)
	{
		vec[i] = 0;
	}
	//XBT_INFO("Migracoes da superstep %d", superstep);
	for (i=0; i< PROCESSES; i++)
	{
		if (list[i].migrate == 1)
		{
			//XBT_INFO("Tentou set=%d, proc=%d, retorno =%d", list[i].set, list[i].process+1, verify_set(NULL, list[i].process+1) );
			if (list[i].set == id && verify_set(NULL, list[i].process+1) != list[i].set)
			{
				target = selecting_destination(vec);
				//XBT_INFO("Processo %d migrou para o recurso %s do set %d", list[i].process,  target, list[i].set);
				contador++;
#ifdef S3				
				retorno = 1;
				MSG_process_migrate(MSG_process_from_PID(list[i].process + 1), MSG_get_host_by_name(target));
				data[list[i].process].id_manager = MSG_process_self_PID();	
#endif
				free(target);
			}
		}
	}
	free(vec);
	XBT_INFO("%d - Migracoes=%d", superstep,contador);
	return retorno;
}
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
int manager(int argc, char *argv[])
{
	int i,j, id, temp, alpha_temp;
	double average=0, t1;
	PM_t *largest;
	unsigned int alpha=ALPHA, alpha_line=0, number_managers, base;
	msg_task_t task_send = NULL, task_recv = NULL;
	char mailbox_send[256], mailbox_recv[256];
	MSG_process_suspend(MSG_process_self());
	number_managers = atoi(MSG_process_get_property_value(MSG_process_self(), "amount"));
	base = atoi(MSG_process_get_property_value(MSG_process_self(), "base"));
	id = atoi(MSG_process_get_property_value(MSG_process_self(), "set"));
	data_manager[id].latency = (double *)malloc(sizeof(double) * number_managers);
	data_manager[id].adaptacao = 0;
	//XBT_INFO("Dados, number_managers=%d, base=%d, meu id=%d", number_managers, base, id);
	for (i=0; i<number_managers; i++)
	{
		if (MSG_process_self_PID() == base + i)
		{
			for (j=0; j < number_managers; j++)
			{
				data_manager[id].latency[j] = 0;
				if (MSG_process_self_PID() != base + j)
				{
					task_send = MSG_task_create("sending" , 0, 10, NULL);
					sprintf(mailbox_send, "process-%d", base + j);
					t1 = MSG_get_clock();
					MSG_task_send(task_send, mailbox_send);
					t1 = MSG_get_clock() - t1;
					data_manager[id].latency[j] = t1;
					task_send = NULL;
				}
			}
		}
		else
		{
			sprintf(mailbox_recv, "process-%d", MSG_process_self_PID());
			MSG_task_recv(&(task_recv), mailbox_recv);
			MSG_task_destroy(task_recv);
			task_recv = NULL;
			//XBT_INFO("Manager %d recebeu dado de %d", id, base + i);
		}	
	}
	for (i=0; i<number_managers; i++)
	{
		if (data_manager[id].latency[i] == 0)
		{
			data_manager[id].latency[i] = 0.000001;
		}
	}
	while(1)
	{
		for (j=0; j<PROCESSES; j++)
		{
			if (data[j].id_manager == MSG_process_get_PID(MSG_process_self()))
			{
				sprintf(mailbox_recv, "process-manager-%d", MSG_process_self_PID());
				MSG_task_recv(&(task_recv), mailbox_recv);
				largest = MSG_task_get_data(task_recv);
				//XBT_INFO("Recebi pm=%f do processo %d", largest->value, largest->process);
				list[largest->process].process = largest->process;
				list[largest->process].value = largest->value;
				list[largest->process].set = largest->set;
				list[largest->process].superstep = largest->superstep;

				list[largest->process].x = largest->x;
				list[largest->process].y = largest->y;
				list[largest->process].z = largest->z;				

				//XBT_INFO("Largest ID: %d em %d, PM: %f, X: %f, Y: %f e Z: %f", largest->process, largest->set,largest->value,largest->x, largest->y, largest->z);	

				free(largest);
				MSG_task_destroy(task_recv);
				task_recv = NULL;
			}
		}
		for (i=0; i<number_managers; i++)
		{
			if (MSG_process_self_PID() == base + i)
			{
				for (j=0; j < number_managers; j++)
				{
					if (MSG_process_self_PID() != base + j)
					{
						task_send = MSG_task_create("sending", 0, 20, NULL);
						sprintf(mailbox_send, "process-%d", base + j);
						MSG_task_send(task_send, mailbox_send);
						task_send = NULL;
					}
				}
			}
			else
			{
				sprintf(mailbox_recv, "process-%d", MSG_process_self_PID());
				MSG_task_recv(&(task_recv), mailbox_recv);
				MSG_task_destroy(task_recv);
				task_recv = NULL;
			}	
		}
		for(alpha_temp=alpha, alpha_line=alpha,i=0; i<alpha; i++)
		{
			average = 0;
			for(j=0; j<PROCESSES; j++)
			{
				average += data[j].vector_superstep_time[i];
				//if (MSG_process_self_PID() == 4)
				//{
				//	XBT_INFO("Tempo da superstep %d do processo %d=%f", i, j, data[j].vector_superstep_time[i]);
				//}
			}
			average = (double) average / (double) PROCESSES;
			for(temp=0, j=0; j<PROCESSES; j++)
			{
				if (data[j].vector_superstep_time[i] < average * (1.0-DELTA) || data[j].vector_superstep_time[i] > average * (1.0 + DELTA) )
				{
					temp++;
				}
			}
			if (temp == 0)
			{
				alpha_line++;
			}
			else
			{
				alpha_line--;
			}
		}
		if (alpha_line < LOWEST_ALPHA)
		{
			alpha=LOWEST_ALPHA;
		}
		else
		{
			alpha=alpha_line;
		}
		verify_migration();
		if (perform_migration(list[0].superstep) == 0)
		{
			data_manager[id].adaptacao = data_manager[id].adaptacao + 1;
			if (data_manager[id].adaptacao >= MAXIMO_SUPERSTEPS_SEM_MIGRAR)
			{
				alpha = alpha_temp *2;
			}
		}
		else
		{
			data_manager[id].adaptacao = 0;
		}
		
		// Sincronizacao entre os processos	
		for (i=0; i<number_managers; i++)
		{
			if (MSG_process_self_PID() == base + i)
			{
				for (j=0; j < number_managers; j++)
				{
					if (MSG_process_self_PID() != base + j)
					{
						task_send = MSG_task_create("sending", 0, 20, NULL);
						sprintf(mailbox_send, "process-sync-%d", base + j);
						MSG_task_send(task_send, mailbox_send);
						task_send = NULL;
					}
				}
			}
			else
			{
				sprintf(mailbox_recv, "process-sync-%d", MSG_process_self_PID());
				MSG_task_recv(&(task_recv), mailbox_recv);
				MSG_task_destroy(task_recv);
				task_recv = NULL;
			}	
		}
		// Fim do codigo novo!!!

		for (j=0; j<PROCESSES; j++)
		{
			if (data[j].id_manager == MSG_process_get_PID(MSG_process_self()))
			{
				task_send = MSG_task_create("sending" , alpha, 1, NULL);
				sprintf(mailbox_send, "process-manager-%d", data[j].id);
				MSG_task_send(task_send, mailbox_send);
				task_send = NULL;
			}
		}
	}
	return 0;
}
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
void *init_vector_communication(int type)
{
	double *vector=NULL;
	int *vectori=NULL;
	int sets, i;
	sets = atoi(MSG_process_get_property_value(MSG_process_from_PID(PROCESSES + 1), "amount"));
	if (type == 0)
	{
		vectori = (int *) malloc(sizeof(int) * sets);
		for(i=0; i< sets;i++)
		{
			vectori[i] = 0;
		}
		return vectori;
	}
	else
	{	
		vector = (double *) malloc(sizeof(double) * sets);
		for(i=0; i< sets;i++)
		{
			vector[i] = 0.0;
		}
	}
	return vector;
}
///////////////////////////////////////////////////////////////////////////////

#if APP1
//Dinamica de Fluidos
///////////////////////////////////////////////////////////////////////////////
int bsp(int argc, char *argv[])
{
	unsigned int i,j, instructions;
	double time_superstep_start, time_superstep_end, time_computation_start, time_computation_end;
	double *time_communication_start;
	int *amount_communication, index;
	msg_task_t task_send = NULL, task_recv = NULL, compute = NULL;
	msg_comm_t async_send, async_recv;
	char mailbox_send[256], mailbox_recv[256];
  init_migbsp();
	time_communication_start = (double *) init_vector_communication(1);	
	amount_communication = (int *)init_vector_communication(0);	
	for (i=0; i< SUPERSTEPS ; i++)
	{
		if (MSG_process_self_PID() == 1)
		{
			XBT_INFO("Superstep %d", i+1);
		}
		time_superstep_start = MSG_get_clock();
		compute =  MSG_task_create("computing", task_comp_size, task_comm_size, NULL);
		time_computation_start = MSG_get_clock();
		MSG_task_execute(compute);
		MSG_task_destroy(compute);
		instructions = task_comp_size;
		time_computation_end = MSG_get_clock();
		if (MSG_process_self_PID() % PROCESSES== 0)
		{
			sprintf(mailbox_send, "process-1");
			sprintf(mailbox_recv, "process-%d", PROCESSES);
		}
		else if (MSG_process_self_PID() == 1)
		{
			sprintf(mailbox_send, "process-2");
			sprintf(mailbox_recv, "process-%d", 1);
		}
		else
		{
			sprintf(mailbox_send, "process-%d", MSG_process_self_PID() + 1);
			sprintf(mailbox_recv, "process-%d", MSG_process_self_PID());
		}
		index = verify_set(mailbox_recv, -1);
		time_communication_start[index] = MSG_get_clock();
		if (MSG_process_self_PID() != PROCESSES)
		{
			task_send = MSG_task_create("sending" , task_comp_size, task_comm_size, NULL);
			async_send = MSG_task_isend(task_send, mailbox_send);
		}
		if (MSG_process_self_PID() != 1)
		{
			amount_communication[index] = task_comm_size;
			async_recv = MSG_task_irecv(&(task_recv), mailbox_recv);
			MSG_comm_wait(async_recv, -1);
			MSG_comm_destroy(async_recv);
			MSG_task_destroy(task_recv);
		}
		time_superstep_end = MSG_get_clock();
		time_communication_start[index] = MSG_get_clock() - time_communication_start[index];
		if (MSG_process_self_PID() != PROCESSES)
		{
			MSG_comm_wait(async_send, -1);
			MSG_comm_destroy(async_send);
		}
		task_recv = NULL;
		task_send = NULL;
		//XBT_INFO("Pronto pra executar a barreira");
		barrier(time_superstep_end - time_superstep_start, time_computation_end - time_computation_start, instructions, time_communication_start, amount_communication, i+1);
		for(j=0; j< number_sets(); j++)
		{
			amount_communication[j] = 0;
		}
	}
	if (MSG_process_self_PID() == 1)
	{
		XBT_INFO("Ending\n");
	}
	finalize_migbsp();
	return 0;
}
#endif
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
	if (argc != 3)
	{
		printf("Use: prog <plat.xml> <depl.xml>\n");
		exit(0);
	}
	MSG_init(&argc, argv);
	MSG_function_register("bsp", bsp);
	MSG_function_register("manager", manager);
	MSG_create_environment(argv[1]);
	MSG_launch_application(argv[2]);
	return MSG_main();
}
///////////////////////////////////////////////////////////////////////////////
