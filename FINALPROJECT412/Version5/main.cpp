//
//  main.c
//  Final Project CSC412
//
//  Created by Jean-Yves Hervé on 2020-12-01
//	This is public domain code.  By all means appropriate it and change is to your
//	heart's content.
#include <iostream>
#include <string>
#include <random>
//
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <climits>
#include <unistd.h>
//
#include "gl_frontEnd.h"

using namespace std;

//==================================================================================
//	Function prototypes
//==================================================================================
void initializeApplication(void);
GridPosition getNewFreePosition(void);
Direction newDirection(Direction forbiddenDir = NUM_DIRECTIONS);
TravelerSegment newTravelerSegment(const TravelerSegment& currentSeg, bool& canAdd);
void generateWalls(void);
void generatePartitions(void);

//==================================================================================
//	Application-level global variables
//==================================================================================

//	Don't rename any of these variables
//-------------------------------------
//	The state grid and its dimensions (arguments to the program)
SquareType** grid;
unsigned int numRows = 0;	//	height of the grid
unsigned int numCols = 0;	//	width
unsigned int numTravelers = 0;	//	initial number
unsigned int numTravelersDone = 0;
unsigned int numLiveThreads = 0;		//	the number of live traveler threads
unsigned int numMovesForGrowth = 0;		// the number of moves before tail growth
vector<Traveler> travelerList;
vector<SlidingPartition> partitionList;
GridPosition	exitPos;	//	location of the exit

//	travelers' sleep time between moves (in microseconds)
const int MIN_SLEEP_TIME = 1000;
int travelerSleepTime = 100000;

//	An array of C-string where you can store things you want displayed
//	in the state pane to display (for debugging purposes?)
//	Dont change the dimensions as this may break the front end
const int MAX_NUM_MESSAGES = 8;
const int MAX_LENGTH_MESSAGE = 32;
char** message;
time_t launchTime;

//	Random generators:  For uniform distributions
const unsigned int MAX_NUM_INITIAL_SEGMENTS = 6;
random_device randDev;
default_random_engine engine(randDev());
uniform_int_distribution<unsigned int> unsignedNumberGenerator(0, numeric_limits<unsigned int>::max());
uniform_int_distribution<unsigned int> segmentNumberGenerator(0, MAX_NUM_INITIAL_SEGMENTS);
uniform_int_distribution<unsigned int> segmentDirectionGenerator(0, NUM_DIRECTIONS-1);
uniform_int_distribution<unsigned int> headsOrTails(0, 1);
uniform_int_distribution<unsigned int> rowGenerator;
uniform_int_distribution<unsigned int> colGenerator;

// Mutex locks
pthread_mutex_t globalLock;
pthread_mutex_t * travelerLocks;
pthread_mutex_t ** gridLocks;

//==================================================================================
//	These are the functions that tie the simulation with the rendering.
//	Some parts are "don't touch."  Other parts need your intervention
//	to make sure that access to critical section is properly synchronized
//==================================================================================

void drawTravelers(void)
{
	//-----------------------------
	//	Obtain global lock before rendering travelers
	//-----------------------------
	pthread_mutex_lock(&globalLock);
		for (unsigned int k=0; k<travelerList.size(); k++)
		{
			pthread_mutex_lock(&travelerLocks[k]);
			if (travelerList[k].pid > 0)
			{
				// Acquire all locks on the traveler's blocks
				for (unsigned int i = 0; i < travelerList[k].segmentList.size(); i++)
				{
					unsigned int row = travelerList[k].segmentList[i].row;
					unsigned int col = travelerList[k].segmentList[i].col;
					pthread_mutex_lock(&gridLocks[row][col]);
				}
				drawTraveler(travelerList[k]);
				// Release all locks on the traveler's blocks
				for (unsigned int i = 0; i < travelerList[k].segmentList.size(); i++)
				{
					unsigned int row = travelerList[k].segmentList[i].row;
					unsigned int col = travelerList[k].segmentList[i].col;
					pthread_mutex_unlock(&gridLocks[row][col]);
				}
			}
			pthread_mutex_unlock(&travelerLocks[k]);
		}
	pthread_mutex_unlock(&globalLock);
}

void updateMessages(void)
{
	//	Obtain global lock before rendering messages
	unsigned int numMessages = 4;
	pthread_mutex_lock(&globalLock);
		sprintf(message[0], "We created %d travelers", numTravelers);
		sprintf(message[1], "%d travelers solved the maze", numTravelersDone);
		sprintf(message[2], "Traveler's sleep time is %d", travelerSleepTime);
		sprintf(message[3], "Simulation run time is %ld", time(NULL)-launchTime);
	pthread_mutex_unlock(&globalLock);
	
	//---------------------------------------------------------
	//	This is the call that makes OpenGL render information
	//	about the state of the simulation.
	//
	//	You *must* synchronize this call.
	//
	//---------------------------------------------------------
	drawMessages(numMessages, message);
}

void handleKeyboardEvent(unsigned char c, int x, int y)
{
	int ok = 0;

	switch (c)
	{
		//	'esc' to quit
		case 27:
			exit(0);
			break;

		//	slowdown
		case ',':
			slowdownTravelers();
			ok = 1;
			break;

		//	speedup
		case '.':
			speedupTravelers();
			ok = 1;
			break;

		default:
			ok = 1;
			break;
	}
	if (!ok)
	{
		//	do something?
	}
}


//------------------------------------------------------------------------
//	You shouldn't have to touch this one.  Definitely if you don't
//	add the "producer" threads, and probably not even if you do.
//------------------------------------------------------------------------
void speedupTravelers(void)
{
	//	decrease sleep time by 20%, but don't get too small
	int newSleepTime = (8 * travelerSleepTime) / 10;
	
	if (newSleepTime > MIN_SLEEP_TIME)
	{
		pthread_mutex_lock(&globalLock);
			travelerSleepTime = newSleepTime;
		pthread_mutex_unlock(&globalLock);
	}
}

void slowdownTravelers(void)
{
	//	increase sleep time by 20%
	pthread_mutex_lock(&globalLock);
		travelerSleepTime = (12 * travelerSleepTime) / 10;
	pthread_mutex_unlock(&globalLock);
}




//------------------------------------------------------------------------
//	You shouldn't have to change anything in the main function besides
//	initialization of the various global variables and lists
//------------------------------------------------------------------------
int main(int argc, char** argv)
{
	//	We know that the arguments  of the program  are going
	//	to be the width (number of columns) and height (number of rows) of the
	//	grid, the number of travelers, etc.
	//  Parse inputs
	if (argc == 4 || argc == 5)
	{
		numRows = atoi(argv[1]);
		numCols = atoi(argv[2]);
		numTravelers = atoi(argv[3]);
		if (argc == 5)
			numMovesForGrowth = atoi(argv[4]);
		else
			numMovesForGrowth = INT_MAX;
	}
	numLiveThreads = 0;
	numTravelersDone = 0;

	// Initialize locks
	pthread_mutex_init(&globalLock, NULL);
	travelerLocks = new pthread_mutex_t[numTravelers];
	for (unsigned int i = 0; i < numTravelers; i++)
		pthread_mutex_init(&travelerLocks[i], NULL);

	// Allocate the grid locks
	gridLocks = new pthread_mutex_t*[numRows];
	for (unsigned int i = 0; i < numRows; i++)
	{
		gridLocks[i] = new pthread_mutex_t[numCols];
		for (unsigned int j = 0; j < numCols; j++)
			pthread_mutex_init(&gridLocks[i][j], NULL);		
	}

	//	Even though we extracted the relevant information from the argument
	//	list, I still need to pass argc and argv to the front-end init
	//	function because that function passes them to glutInit, the required call
	//	to the initialization of the glut library.
	initializeFrontEnd(argc, argv);
	
	//	Now we can do application-level initialization
	initializeApplication();

	launchTime = time(NULL);

	//	Now we enter the main loop of the program and to a large extend
	//	"lose control" over its execution.  The callback functions that 
	//	we set up earlier will be called when the corresponding event
	//	occurs
	glutMainLoop();
	
	//	Free allocated resource before leaving (not absolutely needed, but
	//	just nicer.  Also, if you crash there, you know something is wrong
	//	in your code.
	for (unsigned int i = 0; i < numRows; i++)
		free(grid[i]);
	free(grid);
	for (int k = 0; k < MAX_NUM_MESSAGES; k++)
		free(message[k]);
	free(message);
	free(travelerLocks);
	for (unsigned int i = 0; i < numRows; i++)
		free(gridLocks[i]);
	free(gridLocks);
	
	//	This will probably never be executed (the exit point will be in one of the
	//	call back functions).
	return 0;
}


//==================================================================================
//
//	This is a function that you have to edit and add to.
//
//==================================================================================

void *travelerFunc(void *arg)
{
	// Upldate number of live threads
	pthread_mutex_lock(&globalLock);
		numLiveThreads++;
	pthread_mutex_unlock(&globalLock);
	// Obtain traveler's index
	int index = *(int *)arg;
	// Obtain reference to the current traveler
	Traveler *traveler = &travelerList[index];
	// Inifinitely loop
	while (true)
	{
		// Check if exit condition is reached
		pthread_mutex_lock(&globalLock);
		pthread_mutex_lock(&travelerLocks[index]);
			if (traveler->segmentList[0].row == exitPos.row &&
			   traveler->segmentList[0].col == exitPos.col)
			{
				pthread_mutex_unlock(&travelerLocks[index]);
				pthread_mutex_unlock(&globalLock);				
				break;
			}
			// Obtain head position and direction
			unsigned int row = traveler->segmentList[0].row;
			unsigned int col = traveler->segmentList[0].col;
			Direction dir = traveler->segmentList[0].dir;
		pthread_mutex_unlock(&travelerLocks[index]);
		pthread_mutex_unlock(&globalLock);
		// Find a new available direction except for backward direction
		unsigned int newRow = row;
		unsigned int newCol = col;
		Direction newDir = NUM_DIRECTIONS;
		bool noDir = true;
		while (noDir)
		{
			usleep(100);
			// Get a random direction except for the opposite direction
			newDir = newDirection(static_cast<Direction>((dir + 2) % NUM_DIRECTIONS));
			// Check the validity of the move along this direction in the given grid
			if (newDir == NORTH)
			{
				if (row == 0)
					continue;
				newRow = row - 1;
				newCol = col;
			}
			else if (newDir == SOUTH)
			{
				if (row == numRows - 1)
					continue;
				newRow = row + 1;
				newCol = col;
			}
			else if (newDir == EAST)
			{
				if (col == numCols - 1)
					continue;
				newRow = row;
				newCol = col + 1;
			}
			else if (newDir == WEST)
			{
				if (col == 0)
					continue;
				newRow = row;
				newCol = col - 1;
			}
			else
				continue;
			// Check if the next position is free or exit
			pthread_mutex_lock(&globalLock);
			pthread_mutex_lock(&travelerLocks[index]);
			pthread_mutex_lock(&gridLocks[newRow][newCol]);
				// If free or exit
				if (grid[newRow][newCol] == FREE_SQUARE ||
					grid[newRow][newCol] == EXIT)
					// Set noDir to false, so that the inner while loop exits
					// in the next iteration
					noDir = false;
				// If partition
				else if (grid[newRow][newCol] == VERTICAL_PARTITION ||
						 grid[newRow][newCol] == HORIZONTAL_PARTITION)
				{
					// Find partition index
					SlidingPartition * partition = NULL;
					for (unsigned int i = 0; i < partitionList.size(); i++)
					{
						for (unsigned int j = 0; j < partitionList[i].blockList.size(); j++)
						{
							if (partitionList[i].blockList[j].row == newRow &&
								partitionList[i].blockList[j].col == newCol)
							{
								partition = &partitionList[i];
								break;
							}
						}
						if (partition != NULL)
							break;
					}
					// If not found, continue
					if (partition == NULL)
					{
						// Shouldn't reach here
						pthread_mutex_unlock(&gridLocks[newRow][newCol]);
						pthread_mutex_unlock(&travelerLocks[index]);
						pthread_mutex_unlock(&globalLock);
						continue;
					}
					// Try to move it along its main direction
					// New partition block to add
					unsigned int row1 = 0;
					unsigned int col1 = 0;
					// Current partition block to remove
					unsigned int row2 = 0;
					unsigned int col2 = 0;
					// Shift direction
					int rowStep = 0;
					int colStep = 0;
					// If vertical
					if (partition->isVertical)
					{
						// move top
						if (headsOrTails(engine))
						{
							// Check validity
							if (partition->blockList[0].row == 0)
							{
								pthread_mutex_unlock(&gridLocks[newRow][newCol]);
								pthread_mutex_unlock(&travelerLocks[index]);
								pthread_mutex_unlock(&globalLock);
								continue;
							}
							row1 = partition->blockList[0].row - 1;
							col1 = partition->blockList[0].col;
							row2 = partition->blockList.back().row;
							col2 = partition->blockList.back().col;
							rowStep = -1;
						}
						else
						{
							// Check validity
							if (partition->blockList.back().row == numRows - 1)
							{
								pthread_mutex_unlock(&gridLocks[newRow][newCol]);
								pthread_mutex_unlock(&travelerLocks[index]);
								pthread_mutex_unlock(&globalLock);
								continue;
							}
							row1 = partition->blockList.back().row + 1;
							col1 = partition->blockList.back().col;
							row2 = partition->blockList[0].row;
							col2 = partition->blockList[0].col;
							rowStep = 1;
						}
					}
					// If horizontal
					else
					{
						// move left
						if (headsOrTails(engine))
						{
							// Check validity
							if (partition->blockList[0].col == 0)
							{
								pthread_mutex_unlock(&gridLocks[newRow][newCol]);
								pthread_mutex_unlock(&travelerLocks[index]);
								pthread_mutex_unlock(&globalLock);
								continue;
							}
							row1 = partition->blockList[0].row;
							col1 = partition->blockList[0].col - 1;
							row2 = partition->blockList.back().row;
							col2 = partition->blockList.back().col;
							colStep = -1;
						}
						else
						{
							// Check validity
							if (partition->blockList.back().col == numCols - 1)
							{
								pthread_mutex_unlock(&gridLocks[newRow][newCol]);
								pthread_mutex_unlock(&travelerLocks[index]);
								pthread_mutex_unlock(&globalLock);
								continue;
							}
							row1 = partition->blockList.back().row;
							col1 = partition->blockList.back().col + 1;
							row2 = partition->blockList[0].row;
							col2 = partition->blockList[0].col;
							colStep = 1;
						}
					}
					// Avoid locking again
					if (row1 == newRow && col1 == newCol)
					{
						// Shouldn't reach here
						pthread_mutex_unlock(&gridLocks[newRow][newCol]);
						pthread_mutex_unlock(&travelerLocks[index]);
						pthread_mutex_unlock(&globalLock);
						continue;
					}
					pthread_mutex_lock(&gridLocks[row1][col1]);
						// Avoid locking again
						if ((row2 != newRow || col2 != newCol) && (row2 != row1 || col2 != col1))
						{
							pthread_mutex_lock(&gridLocks[row2][col2]);
						}
						// Check grid position
						if (grid[row1][col1] != FREE_SQUARE)
						{
							// Avoid unlocking again
							if ((row2 != newRow || col2 != newCol) && (row2 != row1 || col2 != col1))
							{
								pthread_mutex_unlock(&gridLocks[row2][col2]);
							}
							pthread_mutex_unlock(&gridLocks[row1][col1]);
							pthread_mutex_unlock(&gridLocks[newRow][newCol]);
							pthread_mutex_unlock(&travelerLocks[index]);
							pthread_mutex_unlock(&globalLock);
							continue;
						}
						// Shift partition
						grid[row2][col2] = FREE_SQUARE;
						if (partition->isVertical)
							grid[row1][col1] = VERTICAL_PARTITION;
						else
							grid[row1][col1] = HORIZONTAL_PARTITION;
						for (unsigned int i = 0; i < partition->blockList.size(); i++)
						{
							partition->blockList[i].row += rowStep;
							partition->blockList[i].col += colStep;
						}
						// Avoid unlocking again
						if ((row2 != newRow || col2 != newCol) && (row2 != row1 || col2 != col1))
						{
							pthread_mutex_unlock(&gridLocks[row2][col2]);
						}
					pthread_mutex_unlock(&gridLocks[row1][col1]);
					pthread_mutex_unlock(&gridLocks[newRow][newCol]);
					pthread_mutex_unlock(&travelerLocks[index]);
					pthread_mutex_unlock(&globalLock);
					continue;
				}
				// Otherwise
				else
				{
					pthread_mutex_unlock(&gridLocks[newRow][newCol]);
					pthread_mutex_unlock(&travelerLocks[index]);
					pthread_mutex_unlock(&globalLock);
					continue;
				}
				// Increase number of moves
				traveler->moves++;
				// Check the number of moves made so far
				// If it is the time to increase the length
				if (traveler->moves == numMovesForGrowth)
				{
					// Add one segment at the back of the list
					TravelerSegment seg = {row, col, dir};
					traveler->segmentList.push_back(seg);
					// Reset counter
					traveler->moves = 0;
				}
				// Otherwise, free the last position
				else
				{
					unsigned int lastRow = traveler->segmentList.back().row;
					unsigned int lastCol = traveler->segmentList.back().col;
					// Avoid locking again
					if (lastRow != newRow || lastCol != newCol)
					{
						pthread_mutex_lock(&gridLocks[lastRow][lastCol]);
					}
					grid[lastRow][lastCol] = FREE_SQUARE;
					// Avoid unlocking again
					if (lastRow != newRow || lastCol != newCol)
					{
						pthread_mutex_unlock(&gridLocks[lastRow][lastCol]);
					}
				}
				// Shift all segments backwards by 1
				for (unsigned int i = traveler->segmentList.size() - 1; i > 0; i--)
					traveler->segmentList[i] = traveler->segmentList[i - 1];
				// Set head at the new position
				traveler->segmentList[0] = {newRow, newCol, newDir};
				if (grid[newRow][newCol] == FREE_SQUARE)
					grid[newRow][newCol] = TRAVELER;
			pthread_mutex_unlock(&gridLocks[newRow][newCol]);
			pthread_mutex_unlock(&travelerLocks[index]);
			pthread_mutex_unlock(&globalLock);
		}
		// Delay
		usleep(travelerSleepTime);
	}
	pthread_mutex_lock(&globalLock);
	pthread_mutex_lock(&travelerLocks[index]);
		// Lock all traveler's blocks
		for (unsigned int i = 1; i < traveler->segmentList.size(); i++)
			pthread_mutex_lock(&gridLocks[traveler->segmentList[i].row][traveler->segmentList[i].col]);
		// Free all squares occupied by traveler
		for (unsigned int i = 1; i < traveler->segmentList.size(); i++)
			grid[traveler->segmentList[i].row][traveler->segmentList[i].col] = FREE_SQUARE;
		// Update global information
		numTravelersDone++;
		numLiveThreads--;
		// Unlock all traveler's blocks
		for (unsigned int i = 1; i < traveler->segmentList.size(); i++)
			pthread_mutex_unlock(&gridLocks[traveler->segmentList[i].row][traveler->segmentList[i].col]);
		// Remove traveler segments all at once
		traveler->segmentList.clear();
		traveler->pid = 0;
	pthread_mutex_unlock(&travelerLocks[index]);
	pthread_mutex_unlock(&globalLock);
	return NULL;
}

void initializeApplication(void)
{
	//	Initialize some random generators
	rowGenerator = uniform_int_distribution<unsigned int>(0, numRows-1);
	colGenerator = uniform_int_distribution<unsigned int>(0, numCols-1);

	//	Allocate the grid
	grid = new SquareType*[numRows];
	for (unsigned int i=0; i<numRows; i++)
	{
		grid[i] = new SquareType[numCols];
		for (unsigned int j=0; j< numCols; j++)
			grid[i][j] = FREE_SQUARE;
		
	}

	message = new char*[MAX_NUM_MESSAGES];
	for (unsigned int k=0; k<MAX_NUM_MESSAGES; k++)
		message[k] = new char[MAX_LENGTH_MESSAGE+1];
		
	//---------------------------------------------------------------
	//	All the code below to be replaced/removed
	//	I initialize the grid's pixels to have something to look at
	//---------------------------------------------------------------
	//	Yes, I am using the C random generator after ranting in class that the C random
	//	generator was junk.  Here I am not using it to produce "serious" data (as in a
	//	real simulation), only wall/partition location and some color
	srand((unsigned int) time(NULL));

	//	generate a random exit
	exitPos = getNewFreePosition();
	grid[exitPos.row][exitPos.col] = EXIT;

	//	Generate walls and partitions
	generateWalls();
	generatePartitions();
	
	//	Initialize traveler info structs
	//	You will probably need to replace/complete this as you add thread-related data
	float** travelerColor = createTravelerColors(numTravelers);
	for (unsigned int k=0; k<numTravelers; k++) {
		GridPosition pos = getNewFreePosition();
		//	Note that treating an enum as a sort of integer is increasingly
		//	frowned upon, as C++ versions progress
		Direction dir = static_cast<Direction>(segmentDirectionGenerator(engine));

		TravelerSegment seg = {pos.row, pos.col, dir};
		Traveler traveler;
		traveler.segmentList.push_back(seg);
		grid[pos.row][pos.col] = TRAVELER;

		//	I add 0-n segments to my travelers
		unsigned int numAddSegments = segmentNumberGenerator(engine);
		TravelerSegment currSeg = traveler.segmentList[0];
		bool canAddSegment = true;
		cout << "Traveler " << k << " at (row=" << pos.row << ", col=" <<
		pos.col << "), direction: " << dirStr(dir) << ", with up to " << numAddSegments << " additional segments" << endl;
		cout << "\t";

		for (unsigned int s=0; s<numAddSegments && canAddSegment; s++)
		{
			TravelerSegment newSeg = newTravelerSegment(currSeg, canAddSegment);
			if (canAddSegment)
			{
				traveler.segmentList.push_back(newSeg);
				currSeg = newSeg;
				cout << dirStr(newSeg.dir) << "  ";
			}
		}
		cout << endl;

		for (unsigned int c=0; c<4; c++)
			traveler.rgba[c] = travelerColor[k][c];
		
		// Initialize traveler's information
		traveler.index = k;
		traveler.moves = 0;

		//  add to the traveler list
		travelerList.push_back(traveler);
	}
	
	//	free array of colors
	for (unsigned int k=0; k<numTravelers; k++)
		delete []travelerColor[k];
	delete []travelerColor;

	// Start traveler threads
	for (unsigned int k=0; k<numTravelers; k++) {
		//  start traveler thread
		pthread_create(&(travelerList[k].pid), NULL, travelerFunc, (void *)&(travelerList[k].index));
	}
}


//------------------------------------------------------
#if 0
#pragma mark -
#pragma mark Generation Helper Functions
#endif
//------------------------------------------------------

GridPosition getNewFreePosition(void)
{
	GridPosition pos;

	bool noGoodPos = true;
	while (noGoodPos)
	{
		unsigned int row = rowGenerator(engine);
		unsigned int col = colGenerator(engine);
		if (grid[row][col] == FREE_SQUARE)
		{
			pos.row = row;
			pos.col = col;
			noGoodPos = false;
		}
	}
	return pos;
}

Direction newDirection(Direction forbiddenDir)
{
	bool noDir = true;

	Direction dir = NUM_DIRECTIONS;
	while (noDir)
	{
		dir = static_cast<Direction>(segmentDirectionGenerator(engine));
		noDir = (dir==forbiddenDir);
	}
	return dir;
}


TravelerSegment newTravelerSegment(const TravelerSegment& currentSeg, bool& canAdd)
{
	TravelerSegment newSeg;
	switch (currentSeg.dir)
	{
		case NORTH:
			if (	currentSeg.row < numRows-1 &&
					grid[currentSeg.row+1][currentSeg.col] == FREE_SQUARE)
			{
				newSeg.row = currentSeg.row+1;
				newSeg.col = currentSeg.col;
				newSeg.dir = newDirection(SOUTH);
				grid[newSeg.row][newSeg.col] = TRAVELER;
				canAdd = true;
			}
			//	no more segment
			else
				canAdd = false;
			break;

		case SOUTH:
			if (	currentSeg.row > 0 &&
					grid[currentSeg.row-1][currentSeg.col] == FREE_SQUARE)
			{
				newSeg.row = currentSeg.row-1;
				newSeg.col = currentSeg.col;
				newSeg.dir = newDirection(NORTH);
				grid[newSeg.row][newSeg.col] = TRAVELER;
				canAdd = true;
			}
			//	no more segment
			else
				canAdd = false;
			break;

		case WEST:
			if (	currentSeg.col < numCols-1 &&
					grid[currentSeg.row][currentSeg.col+1] == FREE_SQUARE)
			{
				newSeg.row = currentSeg.row;
				newSeg.col = currentSeg.col+1;
				newSeg.dir = newDirection(EAST);
				grid[newSeg.row][newSeg.col] = TRAVELER;
				canAdd = true;
			}
			//	no more segment
			else
				canAdd = false;
			break;

		case EAST:
			if (	currentSeg.col > 0 &&
					grid[currentSeg.row][currentSeg.col-1] == FREE_SQUARE)
			{
				newSeg.row = currentSeg.row;
				newSeg.col = currentSeg.col-1;
				newSeg.dir = newDirection(WEST);
				grid[newSeg.row][newSeg.col] = TRAVELER;
				canAdd = true;
			}
			//	no more segment
			else
				canAdd = false;
			break;
		
		default:
			canAdd = false;
	}
	
	return newSeg;
}

void generateWalls(void)
{
	const unsigned int NUM_WALLS = (numCols+numRows)/4;

	//	I decide that a wall length  cannot be less than 3  and not more than
	//	1/4 the grid dimension in its Direction
	const unsigned int MIN_WALL_LENGTH = 3;
	const unsigned int MAX_HORIZ_WALL_LENGTH = numCols / 3;
	const unsigned int MAX_VERT_WALL_LENGTH = numRows / 3;
	const unsigned int MAX_NUM_TRIES = 20;

	bool goodWall = true;
	
	//	Generate the vertical walls
	for (unsigned int w=0; w< NUM_WALLS; w++)
	{
		goodWall = false;
		
		//	Case of a vertical wall
		if (headsOrTails(engine))
		{
			//	I try a few times before giving up
			for (unsigned int k=0; k<MAX_NUM_TRIES && !goodWall; k++)
			{
				//	let's be hopeful
				goodWall = true;
				
				//	select a column index
				unsigned int HSP = numCols/(NUM_WALLS/2+1);
				unsigned int col = (1+ unsignedNumberGenerator(engine)%(NUM_WALLS/2-1))*HSP;
				unsigned int length = MIN_WALL_LENGTH + unsignedNumberGenerator(engine)%(MAX_VERT_WALL_LENGTH-MIN_WALL_LENGTH+1);
				
				//	now a random start row
				unsigned int startRow = unsignedNumberGenerator(engine)%(numRows-length);
				for (unsigned int row=startRow, i=0; i<length && goodWall; i++, row++)
				{
					if (grid[row][col] != FREE_SQUARE)
						goodWall = false;
				}
				
				//	if the wall first, add it to the grid
				if (goodWall)
				{
					for (unsigned int row=startRow, i=0; i<length && goodWall; i++, row++)
					{
						grid[row][col] = WALL;
					}
				}
			}
		}
		// case of a horizontal wall
		else
		{
			goodWall = false;
			
			//	I try a few times before giving up
			for (unsigned int k=0; k<MAX_NUM_TRIES && !goodWall; k++)
			{
				//	let's be hopeful
				goodWall = true;
				
				//	select a column index
				unsigned int VSP = numRows/(NUM_WALLS/2+1);
				unsigned int row = (1+ unsignedNumberGenerator(engine)%(NUM_WALLS/2-1))*VSP;
				unsigned int length = MIN_WALL_LENGTH + unsignedNumberGenerator(engine)%(MAX_HORIZ_WALL_LENGTH-MIN_WALL_LENGTH+1);
				
				//	now a random start row
				unsigned int startCol = unsignedNumberGenerator(engine)%(numCols-length);
				for (unsigned int col=startCol, i=0; i<length && goodWall; i++, col++)
				{
					if (grid[row][col] != FREE_SQUARE)
						goodWall = false;
				}
				
				//	if the wall first, add it to the grid
				if (goodWall)
				{
					for (unsigned int col=startCol, i=0; i<length && goodWall; i++, col++)
					{
						grid[row][col] = WALL;
					}
				}
			}
		}
	}
}


void generatePartitions(void)
{
	const unsigned int NUM_PARTS = (numCols+numRows)/4;

	//	I decide that a partition length  cannot be less than 3  and not more than
	//	1/4 the grid dimension in its Direction
	const unsigned int MIN_PARTITION_LENGTH = 3;
	const unsigned int MAX_HORIZ_PART_LENGTH = numCols / 3;
	const unsigned int MAX_VERT_PART_LENGTH = numRows / 3;
	const unsigned int MAX_NUM_TRIES = 20;

	bool goodPart = true;

	for (unsigned int w=0; w< NUM_PARTS; w++)
	{
		goodPart = false;
		
		//	Case of a vertical partition
		if (headsOrTails(engine))
		{
			//	I try a few times before giving up
			for (unsigned int k=0; k<MAX_NUM_TRIES && !goodPart; k++)
			{
				//	let's be hopeful
				goodPart = true;
				
				//	select a column index
				unsigned int HSP = numCols/(NUM_PARTS/2+1);
				unsigned int col = (1+ unsignedNumberGenerator(engine)%(NUM_PARTS/2-2))*HSP + HSP/2;
				unsigned int length = MIN_PARTITION_LENGTH + unsignedNumberGenerator(engine)%(MAX_VERT_PART_LENGTH-MIN_PARTITION_LENGTH+1);
				
				//	now a random start row
				unsigned int startRow = unsignedNumberGenerator(engine)%(numRows-length);
				for (unsigned int row=startRow, i=0; i<length && goodPart; i++, row++)
				{
					if (grid[row][col] != FREE_SQUARE)
						goodPart = false;
				}
				
				//	if the partition is possible,
				if (goodPart)
				{
					//	add it to the grid and to the partition list
					SlidingPartition part;
					part.isVertical = true;
					for (unsigned int row=startRow, i=0; i<length && goodPart; i++, row++)
					{
						grid[row][col] = VERTICAL_PARTITION;
						GridPosition pos = {row, col};
						part.blockList.push_back(pos);
					}
					partitionList.push_back(part);
				}
			}
		}
		// case of a horizontal partition
		else
		{
			goodPart = false;
			
			//	I try a few times before giving up
			for (unsigned int k=0; k<MAX_NUM_TRIES && !goodPart; k++)
			{
				//	let's be hopeful
				goodPart = true;
				
				//	select a column index
				unsigned int VSP = numRows/(NUM_PARTS/2+1);
				unsigned int row = (1+ unsignedNumberGenerator(engine)%(NUM_PARTS/2-2))*VSP + VSP/2;
				unsigned int length = MIN_PARTITION_LENGTH + unsignedNumberGenerator(engine)%(MAX_HORIZ_PART_LENGTH-MIN_PARTITION_LENGTH+1);
				
				//	now a random start row
				unsigned int startCol = unsignedNumberGenerator(engine)%(numCols-length);
				for (unsigned int col=startCol, i=0; i<length && goodPart; i++, col++)
				{
					if (grid[row][col] != FREE_SQUARE)
						goodPart = false;
				}
				
				//	if the wall first, add it to the grid and build SlidingPartition object
				if (goodPart)
				{
					SlidingPartition part;
					part.isVertical = false;
					for (unsigned int col=startCol, i=0; i<length && goodPart; i++, col++)
					{
						grid[row][col] = HORIZONTAL_PARTITION;
						GridPosition pos = {row, col};
						part.blockList.push_back(pos);
					}
					partitionList.push_back(part);
				}
			}
		}
	}
}

