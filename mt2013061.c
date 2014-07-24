/*
 * Project Name: Dial a Ride
 * Author: Joshi Dnyanesh Madhav
 * Roll No.: MT2013061
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INFINITY 999999
#define NUMBER_OF_MINUTES_IN_A_DAY 1440

typedef struct Location Location;
typedef struct NeighbourDistancePair NeighbourDistancePair;
typedef struct SourcePredecessorPathTriple SourcePredecessorPathTriple;
typedef struct LocationMinuteSeatsTriple LocationMinuteSeatsTriple;
typedef struct Vehicle Vehicle;
typedef struct Request Request;
typedef struct Node Node;
typedef struct List List;
typedef struct QNode QNode;
typedef struct Queue Queue;

struct QNode
{
	LocationMinuteSeatsTriple* lmst;
	QNode* link;
};

struct Queue
{
	QNode* head;
	QNode* tail;
};

struct Location
{
	int index;
	SourcePredecessorPathTriple* sourceDistancePredecessorTriples;
	int numberOfNeighbors;
	NeighbourDistancePair* neighborDistancePairs;
	List* listsOfRequests;
};

struct NeighbourDistancePair
{
	Location* neighbor;
	int distanceToNeighbor;
};

//-ve distanceToSource
//OR numberOfLocationsInShortestPathFromSource==0
//OR locationsInShortestPathFromSource==NULL
//=> location unreachable from source
struct SourcePredecessorPathTriple
{
	Location* source;
	int distanceToSource;
	Location* predecessor;
	int numberOfLocationsInShortestPathFromSource; //both ends inclusive
	Location** locationsInShortestPathFromSource; //both ends inclusive
};

struct LocationMinuteSeatsTriple
{
	Location* location;
	int minuteOfDay;
	List* seats;
};

struct Vehicle
{
	int index;
	Location* base;
	Queue* queueOfLocationMinuteSeatsTriples;
	int revenue;
};

//fare<0 => unservable request
struct Request
{
	int index;
	Location* source;
	Location* destination;
	int beginMinute;
	int endMinute;
	int pickUpMinute;
	int dropOffMinute;
	Vehicle* vehicle;
	int fare;
};

struct Node
{
	Request* request;
	Node* link;
};

struct List
{
	Node* head; //max, if sorted
};

FILE* ifp;
int numberOfLocations;
int numberOfVehicles;
int vehicleCapacity;
int numberOfRequests;
int farePerKm=1;
int numberOfMinutesPerKm=2;
Location* locations;
Vehicle* vehicles;
Request* requests;

Queue* createQueue()
{
	Queue* q;
	q=(Queue*)calloc(1, sizeof(Queue));
	q->head=NULL;
	q->tail=NULL;
	return q;
}

void enqueue(Queue* queue, void* lmst)
{
	QNode* n;
	
	n=(QNode*)calloc(1, sizeof(QNode));
	n->lmst=lmst;
	n->link=NULL;
	
	if(queue->head==NULL)
	{
		queue->tail=n;
		queue->head=n;
	}
	else
	{
		queue->tail->link=n;
		queue->tail=n;
	}
}

void* dequeue(Queue* queue)
{
	void* p;
	QNode* n;
	if(queue->head==NULL)
		return NULL;
	else
	{
		p=queue->head->lmst;
		n=queue->head;
		queue->head=queue->head->link;
		if(queue->head==NULL)
			queue->tail=NULL;
		free(n);
		return p;
	}
}

int getQueueLength(Queue* queue)
{
	int len=1;
	QNode* n=queue->head;
	if(n==NULL)
		return 0;
	while(n!=queue->tail)
	{
		len++;
		n=n->link;
	}
	return len;
}

void* getElementInQueue(Queue* queue, int index)
{
	int i;
	QNode* n;
	if(index>=getQueueLength(queue))
		return NULL;
	n=queue->head;
	for(i=0; i<index; i++)
		n=n->link;
	return n->lmst;
}

void deleteQueue(Queue* q)
{
	while(dequeue(q)!=NULL);
}

void insertIntoList(List* list, Request* request)
{
	Node* n;
	if(request->fare>0)
	{
		n=(Node*)calloc(1, sizeof(Node));
		n->request=request;
		n->link=list->head;
		list->head=n;
	}
}

int getListLength(List* list)
{
	int len=0;
	Node* n;
	n=list->head;
	while(n!=NULL)
	{
		len++;
		n=n->link;
	}
	return len;
}

void sortList(List* list)
{
	Node *p, *q;
	Request* temp;
	p=list->head;
	if(p!=NULL && p->link!=NULL)
	{
		while(p->link!=NULL)
		{
			q=p->link;
			while(q!=NULL)
			{
				if(p->request->fare<q->request->fare)
				{
					temp=p->request;
					p->request=q->request;
					q->request=temp;
				}
				q=q->link;
			}
			p=p->link;
		}
	}
}

int getDistanceBetweenNeighbors(Location* location, Location* neighbor)
{
	int i;
	for(i=0; i<location->numberOfNeighbors; i++)
		if(location->neighborDistancePairs[i].neighbor==neighbor)
			return location->neighborDistancePairs[i].distanceToNeighbor;
	return -1; //ideally unreachable, will never happen unless neighbor isn't a neighbor of location
}

void computeSingleSourceShortestDistances(Location* source)
{
	static int numberOfProcessedLocations=0;
	int i, j, k, d;
	Location** la=(Location**)calloc(numberOfLocations, sizeof(Location*));
	Location* l;
	
	for(i=0; i<numberOfLocations; i++)
	{
		locations[i].sourceDistancePredecessorTriples[numberOfProcessedLocations].source=source;
		locations[i].sourceDistancePredecessorTriples[numberOfProcessedLocations].distanceToSource=INFINITY;
		locations[i].sourceDistancePredecessorTriples[numberOfProcessedLocations].predecessor=NULL;
		locations[i].sourceDistancePredecessorTriples[numberOfProcessedLocations].numberOfLocationsInShortestPathFromSource=0;
		locations[i].sourceDistancePredecessorTriples[numberOfProcessedLocations].locationsInShortestPathFromSource=NULL;
	}
	source->sourceDistancePredecessorTriples[numberOfProcessedLocations].distanceToSource=0;
	source->sourceDistancePredecessorTriples[numberOfProcessedLocations].numberOfLocationsInShortestPathFromSource=1;
	source->sourceDistancePredecessorTriples[numberOfProcessedLocations].locationsInShortestPathFromSource=(Location**)calloc(1, sizeof(Location*));
	source->sourceDistancePredecessorTriples[numberOfProcessedLocations].locationsInShortestPathFromSource[0]=source;
	
	
	//Bellman Ford Algorithm
	for(k=0; k<numberOfLocations-1; k++)
	{
		for(j=0; j<numberOfLocations; j++)
		{
			for(i=0; i<locations[j].numberOfNeighbors; i++)
			{
				d=locations[j].sourceDistancePredecessorTriples[numberOfProcessedLocations].distanceToSource
					+getDistanceBetweenNeighbors(&locations[j], locations[j].neighborDistancePairs[i].neighbor);
					
				if(locations[j].neighborDistancePairs[i].neighbor->sourceDistancePredecessorTriples[numberOfProcessedLocations].distanceToSource>d)
				{
					locations[j].neighborDistancePairs[i].neighbor->sourceDistancePredecessorTriples[numberOfProcessedLocations].predecessor=&locations[j];
					locations[j].neighborDistancePairs[i].neighbor->sourceDistancePredecessorTriples[numberOfProcessedLocations].distanceToSource=d;
				}
			}
		}
	}
	
	source->sourceDistancePredecessorTriples[numberOfProcessedLocations].distanceToSource=0; //just making sure
	source->sourceDistancePredecessorTriples[numberOfProcessedLocations].predecessor=NULL; //just making sure
	
	for(i=0; i<numberOfLocations; i++)
	{
		if(locations[i].sourceDistancePredecessorTriples[numberOfProcessedLocations].distanceToSource>=INFINITY)
		{
			locations[i].sourceDistancePredecessorTriples[numberOfProcessedLocations].predecessor=NULL;
			locations[i].sourceDistancePredecessorTriples[numberOfProcessedLocations].distanceToSource=-1;
		}
		else if(locations[i].sourceDistancePredecessorTriples[numberOfProcessedLocations].distanceToSource>0)
		{
			j=0;
			l=&locations[i];
			while(l!=source)
			{
				la[j++]=l;
				l=l->sourceDistancePredecessorTriples[numberOfProcessedLocations].predecessor;
			}
			la[j++]=source;
			
			locations[i].sourceDistancePredecessorTriples[numberOfProcessedLocations].numberOfLocationsInShortestPathFromSource=j;
			locations[i].sourceDistancePredecessorTriples[numberOfProcessedLocations].locationsInShortestPathFromSource=(Location**)calloc(j, sizeof(Location*));
			j--; k=0;
			while(j>=0)
				locations[i].sourceDistancePredecessorTriples[numberOfProcessedLocations].locationsInShortestPathFromSource[k++]=la[j--];
		}
	}
	
	free(la);
	numberOfProcessedLocations++;
}

void computeAllPairsShortestDistances()
{
	int i;
	for(i=0; i<numberOfLocations; i++)
	{
		computeSingleSourceShortestDistances(&locations[i]);
	}
}

int getShortestDistanceBetweenTwoLocations(Location* source, Location* destination)
{
	int i;
	for(i=0; i<numberOfLocations; i++)
	{
		if(destination->sourceDistancePredecessorTriples[i].source==source)
			return destination->sourceDistancePredecessorTriples[i].distanceToSource;
	}
	return -1; //ideally unreachable
}

int getMinutesToTravelShortestDistance(Location* source, Location* destination)
{
	return numberOfMinutesPerKm*getShortestDistanceBetweenTwoLocations(source, destination);
}

Location** getShortestPathBetweenTwoLocations(Location* source, Location* destination)
{
	int i;
	for(i=0; i<numberOfLocations; i++)
	{
		if(destination->sourceDistancePredecessorTriples[i].source==source)
			return destination->sourceDistancePredecessorTriples[i].locationsInShortestPathFromSource;
	}
	return NULL; //ideally unreachable
}

int getNumberOfLocationsInShortestPath(Location* source, Location* destination)
{
	int i;
	for(i=0; i<numberOfLocations; i++)
	{
		if(destination->sourceDistancePredecessorTriples[i].source==source)
			return destination->sourceDistancePredecessorTriples[i].numberOfLocationsInShortestPathFromSource;
	}
	return 0; //ideally unreachable
}

int isLocationPresentInShortestPath(Location* source, Location* destination, Location* needle)
{
	int i;
	Location** l=getShortestPathBetweenTwoLocations(source, destination);
	for(i=0; i<getNumberOfLocationsInShortestPath(source, destination); i++)
		if(l[i]==needle)
			return 1;
	return 0;
}

void createGraph()
{
	int i, j, k;
	int* distancesToNeighbors;
	
	locations=(Location*)calloc(numberOfLocations, sizeof(Location));
	for(i=0; i<numberOfLocations; i++)
	{
		locations[i].index=i+1;
		locations[i].sourceDistancePredecessorTriples=(SourcePredecessorPathTriple*)calloc(numberOfLocations, sizeof(SourcePredecessorPathTriple));
		locations[i].listsOfRequests=(List*)calloc(NUMBER_OF_MINUTES_IN_A_DAY, sizeof(List));
		for(j=0; j<NUMBER_OF_MINUTES_IN_A_DAY; j++)
			locations[i].listsOfRequests[j].head=NULL;
	}
	distancesToNeighbors=(int*)calloc(numberOfLocations, sizeof(int));
	for(i=0; i<numberOfLocations; i++)
	{
		locations[i].numberOfNeighbors=0;
		for(j=0; j<numberOfLocations; j++)
		{
			fscanf(ifp, "%d", &distancesToNeighbors[j]);
			if(distancesToNeighbors[j]>-1)
				locations[i].numberOfNeighbors++;
		}
		
		locations[i].neighborDistancePairs=(NeighbourDistancePair*)calloc(locations[i].numberOfNeighbors, sizeof(NeighbourDistancePair));
		
		k=0;
		for(j=0; j<numberOfLocations; j++)
		{
			if(distancesToNeighbors[j]>-1)
			{
				locations[i].neighborDistancePairs[k].neighbor=&locations[j];
				locations[i].neighborDistancePairs[k].distanceToNeighbor=distancesToNeighbors[j];
				k++;
			}
		}
	}
	free(distancesToNeighbors);
	computeAllPairsShortestDistances();
}

void deleteGraph()
{
	int i, j;
	Node *p, *q;
	for(i=0; i<numberOfLocations; i++)
	{
		free(locations[i].sourceDistancePredecessorTriples);
		free(locations[i].neighborDistancePairs);
		for(j=0; j<NUMBER_OF_MINUTES_IN_A_DAY; j++)
		{
			p=locations[i].listsOfRequests[j].head;
			while(p!=NULL)
			{
				q=p;
				p=p->link;
				free(q);
			}
		}
		free(locations[i].listsOfRequests);
	}
	free(locations);
}

void createVehicles()
{
	int i, baseIndex;
	vehicles=(Vehicle*)calloc(numberOfVehicles, sizeof(Vehicle));
	for(i=0; i<numberOfVehicles; i++)
	{
		vehicles[i].index=i+1;
		fscanf(ifp, "%d", &baseIndex);
		vehicles[i].base=&locations[baseIndex-1];
		vehicles[i].queueOfLocationMinuteSeatsTriples=createQueue();
		vehicles[i].revenue=0;
	}
}

void deleteVehicles()
{
	int i;
	for(i=0; i<numberOfVehicles; i++)
		deleteQueue(vehicles[i].queueOfLocationMinuteSeatsTriples);
	free(vehicles);
}

void createRequests()
{
	int i, srcIndex, destIndex;
	requests=(Request*)calloc(numberOfRequests, sizeof(Request));
	for(i=0; i<numberOfRequests; i++)
	{
		requests[i].index=i+1;
		fscanf(ifp, "%d%d%d%d", &srcIndex, &destIndex, &requests[i].beginMinute, &requests[i].endMinute);
		requests[i].source=&locations[srcIndex-1];
		requests[i].destination=&locations[destIndex-1];
		requests[i].pickUpMinute=-1;
		requests[i].dropOffMinute=-1;
		requests[i].vehicle=NULL;
		requests[i].fare=farePerKm*getShortestDistanceBetweenTwoLocations(requests[i].source, requests[i].destination);
	}
}

void deleteRequests()
{
	free(requests);
}

void populateAndSortLists()
{
	int i, j;
	for(i=0; i<numberOfRequests; i++)
		if(requests[i].fare>0)
			for(j=requests[i].beginMinute; j<=requests[i].endMinute; j++)
				insertIntoList(&requests[i].source->listsOfRequests[j], &requests[i]);
	for(i=0; i<numberOfLocations; i++)
		for(j=0; j<NUMBER_OF_MINUTES_IN_A_DAY; j++)
			sortList(&locations[i].listsOfRequests[j]);
}

void initMemory(char* fPath)
{
	if((ifp=fopen(fPath, "r"))==NULL)
	{
		printf("Error--The specified input file does not exist. Please pass a valid file path as the argument.\nProgram terminated.\n");
		exit(0);
	}
	fscanf(ifp, "%d", &numberOfLocations);
	fscanf(ifp, "%d", &numberOfVehicles);
	fscanf(ifp, "%d", &vehicleCapacity);
	fscanf(ifp, "%d", &numberOfRequests);
	createGraph();
	createVehicles();
	createRequests();
	populateAndSortLists();
}

void freeMemory()
{
	deleteRequests();
	deleteVehicles();
	deleteGraph();
	fclose(ifp);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
Request* getPartiallyOverlappingRequest(Location* location, int minuteOfDay, Location* mustVisit)
{
	int t;
	Node* n=location->listsOfRequests[minuteOfDay].head;
	while(n!=NULL)
	{
		t=getMinutesToTravelShortestDistance(location, n->request->destination);
		if(n->request->vehicle==NULL && t>0 && minuteOfDay+t<NUMBER_OF_MINUTES_IN_A_DAY && isLocationPresentInShortestPath(location, n->request->destination, mustVisit))
			return n->request;
		n=n->link;
	}
	return NULL;
}

Request* getSubRequest(Location* location, int minuteOfDay, Location* mustVisit)
{
	Node* n=location->listsOfRequests[minuteOfDay].head;
	while(n!=NULL)
	{
		if(n->request->vehicle==NULL && isLocationPresentInShortestPath(location, mustVisit, n->request->destination))
			return n->request;
		n=n->link;
	}
	return NULL;
}

void updateRequest(Request* request, int pickUpMinute, Vehicle* vehicle)
{
	request->pickUpMinute=pickUpMinute;
	request->vehicle=vehicle;
}

void updateVehicle(Vehicle* vehicle, int revenue, LocationMinuteSeatsTriple* lmst)
{
	vehicle->revenue+=revenue;
	enqueue(vehicle->queueOfLocationMinuteSeatsTriples, lmst);
}

Location* getSuitableLocation(Location* location, int minuteOfDay, int* minutesToFirstRequestAtSuitLoc)
{
	int i, j, minutesToReach, bestRequestMinute, maxFareTimeDifference=-1*INFINITY;
	Node* n;
	Location* l;
	for(i=0; i<numberOfLocations; i++)
	{
		minutesToReach=getMinutesToTravelShortestDistance(location, &locations[i]);
		if(minutesToReach<0)
			continue;
		for(j=minuteOfDay+minutesToReach; j<NUMBER_OF_MINUTES_IN_A_DAY; j++)
		{
			n=locations[i].listsOfRequests[j].head;
			if(n!=NULL && n->request->vehicle==NULL && j+getMinutesToTravelShortestDistance(&locations[i], n->request->destination)<NUMBER_OF_MINUTES_IN_A_DAY)
			{
				if((n->request->fare-(j-minuteOfDay))>maxFareTimeDifference)
				{
					maxFareTimeDifference=n->request->fare-(j-minuteOfDay);
					bestRequestMinute=j;
					l=&locations[i];
				}
			}
		}
	}
	if(bestRequestMinute>=NUMBER_OF_MINUTES_IN_A_DAY)
		return NULL;
	*minutesToFirstRequestAtSuitLoc=bestRequestMinute-getMinutesToTravelShortestDistance(location, l)-minuteOfDay;
	return l;
}

void visit(Vehicle* vehicle, Location* location, int minuteOfDay, Location* mustVisit)
{
	List* occupiedSeats;
	Node* seat;
	Location* nextLocation;
	int revenue=0, population=0, minutesToWaitHere=0;
	Request* request=NULL;
	LocationMinuteSeatsTriple* lmst=(LocationMinuteSeatsTriple*)calloc(1, sizeof(LocationMinuteSeatsTriple));
	lmst->location=location;
	lmst->minuteOfDay=minuteOfDay;
	lmst->seats=(List*)calloc(1, sizeof(List));
	printf("\n%d\t\t|  %d\t\t|  ", minuteOfDay, location->index);
	if(getQueueLength(vehicle->queueOfLocationMinuteSeatsTriples)>0)
	{
		occupiedSeats=vehicle->queueOfLocationMinuteSeatsTriples->tail->lmst->seats;
		seat=occupiedSeats->head;
		while(seat!=NULL)
		{
			if(seat->request->destination!=location)
			{
				insertIntoList(lmst->seats, seat->request);
				printf("%d(C) ", seat->request->index);
				population++;
			}
			else
			{
				seat->request->dropOffMinute=minuteOfDay;
				printf("%d(D) ", seat->request->index);
			}
			seat=seat->link;
		}
	}
	while(population<vehicleCapacity && (request=getPartiallyOverlappingRequest(location, minuteOfDay, mustVisit))!=NULL)
	{
		updateRequest(request, minuteOfDay, vehicle);
		insertIntoList(lmst->seats, request);
		printf("%d(P) ", request->index);
		revenue+=request->fare;
		population++; //useful in the next if condition
		mustVisit=request->destination;
	}
	while(population<vehicleCapacity && (request=getSubRequest(location, minuteOfDay, mustVisit))!=NULL)
	{
		updateRequest(request, minuteOfDay, vehicle);
		insertIntoList(lmst->seats, request);
		printf("%d(P) ", request->index);
		revenue+=request->fare;
		population++; //no use actually
	}
	printf("[Population=%d", population);
	updateVehicle(vehicle, revenue, lmst);
	if(location==mustVisit)
	{
		mustVisit=getSuitableLocation(location, minuteOfDay, &minutesToWaitHere);
		minuteOfDay+=minutesToWaitHere;
		if(minutesToWaitHere>0)
			printf(", Halt Time=%d", minutesToWaitHere);
	}
	printf("]");
	if(mustVisit!=location && mustVisit!=NULL)
	{
		nextLocation=getShortestPathBetweenTwoLocations(location, mustVisit)[1];
		minuteOfDay+=getMinutesToTravelShortestDistance(location, nextLocation);
		if(minuteOfDay<NUMBER_OF_MINUTES_IN_A_DAY && nextLocation!=NULL)
			visit(vehicle, nextLocation, minuteOfDay, mustVisit);
	}
	else if(minuteOfDay<NUMBER_OF_MINUTES_IN_A_DAY && mustVisit==location && mustVisit!=NULL && location->listsOfRequests[minuteOfDay].head!=NULL)
		visit(vehicle, location, minuteOfDay, location);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
	int i, j, sum, count, totalReqCount;
	
	if(argc!=2)
	{
		printf("Error--Invalid number of arguments. Please pass the input file path as the argument.\nProgram terminated.\n");
		exit(0);
	}
	
	initMemory(argv[1]);
	
	for(i=0; i<numberOfVehicles; i++)
	{
		printf("\n\n*********************************************************************************");
		printf("\n**** VEHICLE %d ****", vehicles[i].index);
		printf("\n*********************************************************************************");
		printf("\nMin\t\t|  Location\t|  Requests (Status: P-Pick, C-Continue carrying, D-Drop)");
		printf("\n   \t\t|          \t|  [Vehicle Population, Halt Time (if any)]");
		printf("\n---------------------------------------------------------------------------------");
		visit(&vehicles[i], vehicles[i].base, 0, vehicles[i].base);
		printf("\n_________________________________________________________________________________");
		printf("\n\nSUMMARY OF REQUESTS VEHICLE %d WILL SERVE", vehicles[i].index);
		printf("\n----------------------------------------");
		printf("\nReq\t\t|  Fare");
		printf("\n-----------------------");
		for(j=0; j<numberOfRequests; j++)
			if(requests[j].vehicle==&vehicles[i])
				printf("\n%d\t\t|  %d", requests[j].index, requests[j].fare);
		printf("\n-----------------------");
		printf("\nRevenue = %d", vehicles[i].revenue);
		printf("\n_______________________\n");
	}
	
	printf("\n\n*********************************************************************************");
	printf("\n**** REJECTED REQUESTS ****");
	printf("\n*********************************************************************************\n");
	count=0;
	for(i=0; i<numberOfRequests; i++)
		if(requests[i].vehicle==NULL)
		{
			printf("%d  ", requests[i].index);
			count++;
		}
	printf("\n---------------------------------------------------------------------------------");
	printf("\nTotal Number of Rejected Requests = %d", count);
	printf("\n_________________________________________________________________________________\n");
	
	printf("\n\n*********************************************************************************");
	printf("\n**** TOTAL REVENUE CALCULATION ****");
	printf("\n*********************************************************************************");
	sum=0; totalReqCount=0;
	for(i=0; i<numberOfVehicles; i++)
	{
		printf("\nVehicle %d's Revenue = %d", vehicles[i].index, vehicles[i].revenue);
		count=0;
		for(j=0; j<numberOfRequests; j++)
			if(requests[j].vehicle==&vehicles[i])
				count++;
		printf(" [Number of Requests Vehicle %d Will Serve = %d]", vehicles[i].index, count);
		sum+=vehicles[i].revenue;
		totalReqCount+=count;
	}
	printf("\n---------------------------------------------------------------------------------");
	printf("\nTotal Revenue = %d [Total Number of Requests that Will be Served = %d]", sum, totalReqCount);
	printf("\n_________________________________________________________________________________\n\n");
	
	freeMemory();
	return 0;
}
