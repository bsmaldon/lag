
#include "quadtree.h"


#include <stdlib.h>


#include <sstream>

#include "cacheminder.h"
#include "boost/filesystem.hpp"

#include "time.h"

#include "collisiondetection.h"

using namespace std;

void quadtree::initilisevalues(int cap, int cachesize, int depth, int resolutionbase, int numresolutionlevels, ostringstream *errorstream)
{
   if(resolutionbase < 1)
   {
      throw "invalid resolution base";
   }
   if(numresolutionlevels < 1)
   {
      throw "invalid number of resolution levels";
   }
   if (errorstream == NULL)
   {
      // if no stringstream is given it defaults to the error stream
      this->errorstream = &cerr;
   }
   else
   {
      this->errorstream = errorstream;
   }

   capacity = cap;
   this->resolutionbase = resolutionbase;
   this->numresolutionlevels = numresolutionlevels;
   root = NULL;
   guessbucket = NULL;
   instancedirectory = "/tmp/lag_";
   instancedirectory.append(boost::lexical_cast<string > (time(NULL)));
   instancedirectory.append(boost::lexical_cast<string > (getpid()));
   instancedirectory.append(boost::lexical_cast<string > (this));
   boost::filesystem::create_directory(instancedirectory);
   MCP = new cacheminder(cachesize);
   flightlinenum = 0;
   prebuilddepth = depth;
}

// this constructor creates a quadtree using the parameters given. it then loads
// into the quadtree the lidarpointloader that was passed
quadtree::quadtree(lidarpointloader *loader, int cap, int nth, int cachesize, int depth, int resolutionbase, int numresolutionlevels, ostringstream *errorstream)
{
   initilisevalues(cap, cachesize, depth, resolutionbase, numresolutionlevels, errorstream);
   // get the boundary of the file points
   boundary *b = loader->getboundary();
   // use boundary to create new tree that incompasses all points
   root = new quadtreenode(b->minX, b->minY, b->maxX, b->maxY, capacity, MCP, instancedirectory, resolutionbase, numresolutionlevels);
   root->increasedepth(depth);
   load(loader, nth);
}



// this constructor creates a quadtree using a loader object for a given area of interest

quadtree::quadtree(lidarpointloader *loader, int cap, int nth, double *Xs, double *Ys, int size, int cachesize, int depth, int resolutionbase, int numresolutionlevels, ostringstream *errorstream)
{
  initilisevalues(cap, cachesize, depth, resolutionbase, numresolutionlevels, errorstream);
  

   // find the simple bounding box of the new fence
   double maxX, maxY, minX, minY;
   maxX=Xs[0]; minX=Xs[0]; maxY=Ys[0]; minY=Ys[0];
   for (int k=1; k<size; k++)
   {
      if(Xs[k] > maxX) {maxX=Xs[k];}
      if(Xs[k] < minX) {minX=Xs[k];}
      if(Ys[k] > maxY) {maxY=Ys[k];}
      if(Ys[k] < minY) {minY=Ys[k];}
   }
   root = new quadtreenode(minX, minY, maxX, maxY, capacity, MCP, instancedirectory, resolutionbase, numresolutionlevels);
   root->increasedepth(depth);
   // use area of intrest load
   load(loader, nth, Xs, Ys, size);

}


quadtree::quadtree(boundary b, int cap, int cachesize, int depth, int resolutionbase, int numresolutionlevels, ostringstream *errorstream)
{
   quadtree(b.minX, b.minY, b.maxX, b.maxY, cap, cachesize, depth, resolutionbase, numresolutionlevels, errorstream);
}


// this constructor creates an empty quadtree to the input specifications
// NOTE: this could still have data loaded into if using load but
// the points may not fail within the boundry
quadtree::quadtree(double minX, double minY, double maxX, double maxY, int cap, int cachesize, int depth, int resolutionbase, int numresolutionlevels, ostringstream *errorstream)
{
   initilisevalues(cap, cachesize, depth, resolutionbase, numresolutionlevels, errorstream);
   root = new quadtreenode(minX, minY, maxX, maxY, capacity, MCP, instancedirectory, resolutionbase, numresolutionlevels);
   root->increasedepth(depth);
}


// this method expands a quadtree to encompass a new boundary
quadtreenode* quadtree::expandboundary(quadtreenode* oldnode, boundary* nb)
{

   

   boundary* b = oldnode->getbound();

   // if the node already covers the area just return the node
   if (b->maxX == nb->maxX && b->maxY == nb->maxY && b->minX == nb->minX && b->minY == nb->minY)
   {
      return oldnode;
   }


   double newbx1 = b->minX;
   double newby1 = b->minY;
   double newbx2 = b->maxX;
   double newby2 = b->maxY;

   // find the boundary that encompasses the old node and the new boundary
   if (nb->minX < newbx1)
   {
      newbx1 = nb->minX;
   }
   if (nb->minY < newby1)
   {
      newby1 = nb->minY;
   }
   if (nb->maxX > newbx2)
   {
      newbx2 = nb->maxX;
   }
   if (nb->maxY > newby2)
   {
      newby2 = nb->maxY;
   }

   // work out the center point of the new boundary 
   double cx = newbx1 + ((newbx2 - newbx1) / 2);
   double cy = newby1 + ((newby2 - newby1) / 2);

   // find the distance from each of the 4 corners of the nodes boundary to the new center point
   // (work out the general location of the node within the new boundary (top left, top right etc))
   double topleftdistance = sqrt(pow(abs(cx - b->minX), 2) + pow((abs(cy - b->maxY)), 2));
   double toprightdistance = sqrt(pow(abs(cx - b->maxX), 2) + pow((abs(cy - b->maxY)), 2));
   double bottomleftdistance = sqrt(pow(abs(cx - b->minX), 2) + pow((abs(cy - b->minY)), 2));
   double bottomrightdistance = sqrt(pow(abs(cx - b->maxX), 2) + pow((abs(cy - b->minY)), 2));

   // if the old node is in the bottom right
   if (topleftdistance <= toprightdistance && topleftdistance <= bottomleftdistance && topleftdistance <= bottomrightdistance)
   {
      // create nodes that divide up the new boundary with the dividing lines passing through 
      // the top left corner of the old node
      quadtreenode* tl = new quadtreenode(newbx1, b->maxY, b->minX, newby2, capacity, MCP, instancedirectory, resolutionbase, numresolutionlevels);
      quadtreenode* tr = new quadtreenode(b->minX, b->maxY, newbx2, newby2, capacity, MCP, instancedirectory, resolutionbase, numresolutionlevels);
      quadtreenode* bl = new quadtreenode(newbx1, newby1, b->minX, b->maxY, capacity, MCP, instancedirectory, resolutionbase, numresolutionlevels);

      boundary* subboundary = new boundary;
      subboundary->minX = b->minX;
      subboundary->minY = newby1;
      subboundary->maxX = newbx2;
      subboundary->maxY = b->maxY;

      // the old node then needs to be expanded into its new quarter (this is to deal with
      // instances where the old node only touches one side of the new boundary and 
      // therefore only fills half its new quarter.
      quadtreenode* br = expandboundary(oldnode, subboundary);
      delete subboundary;
      delete b;
      // create a new node above the old containing the 3 new child nodes and the expaned old node
      return new quadtreenode(newbx1, newby1, newbx2, newby2, capacity, tl, tr, bl, br, MCP, instancedirectory, resolutionbase, numresolutionlevels);
   }

   // if the old node is in the bottom left
   if (toprightdistance <= topleftdistance && toprightdistance <= bottomleftdistance && toprightdistance <= bottomrightdistance)
   {
      // create nodes that divide up the new boundary with the dividing lines passing through 
      // the top right corner of the old node
      quadtreenode* tl = new quadtreenode(newbx1, b->maxY, b->maxX, newby2, capacity, MCP, instancedirectory, resolutionbase, numresolutionlevels);
      quadtreenode* tr = new quadtreenode(b->maxX, b->maxY, newbx2, newby2, capacity, MCP, instancedirectory, resolutionbase, numresolutionlevels);

      boundary* subboundary = new boundary;
      subboundary->minX = newbx1;
      subboundary->minY = newby1;
      subboundary->maxX = b->maxX;
      subboundary->maxY = b->maxY;

      // the old node then needs to be expanded into its new quarter
      quadtreenode* bl = expandboundary(oldnode, subboundary);
      delete subboundary;
      quadtreenode* br = new quadtreenode(b->maxX, newby1, newbx2, b->maxY, capacity, MCP, instancedirectory, resolutionbase, numresolutionlevels);
      delete b;
      return new quadtreenode(newbx1, newby1, newbx2, newby2, capacity, tl, tr, bl, br, MCP, instancedirectory, resolutionbase, numresolutionlevels);
   }

   // if the old node is in the top right
   if (bottomleftdistance <= topleftdistance && bottomleftdistance <= toprightdistance && bottomleftdistance <= bottomrightdistance)
   {
      // create nodes that divide up the new boundary with the dividing lines passing through 
      // the bottom left corner of the old node
      quadtreenode* tl = new quadtreenode(newbx1, b->minY, b->minX, newby2, capacity, MCP, instancedirectory, resolutionbase, numresolutionlevels);

      boundary* subboundary = new boundary;
      subboundary->minX = b->minX;
      subboundary->minY = b->minY;
      subboundary->maxX = newbx2;
      subboundary->maxY = newby2;

      // the old node then needs to be expanded into its new quarter
      quadtreenode* tr = expandboundary(oldnode, subboundary);
      delete subboundary;
      quadtreenode* bl = new quadtreenode(newbx1, newby1, b->minX, b->minY, capacity, MCP, instancedirectory, resolutionbase, numresolutionlevels);
      quadtreenode* br = new quadtreenode(b->minX, newby1, newbx2, b->minY, capacity, MCP, instancedirectory, resolutionbase, numresolutionlevels);
      delete b;
      return new quadtreenode(newbx1, newby1, newbx2, newby2, capacity, tl, tr, bl, br, MCP, instancedirectory, resolutionbase, numresolutionlevels);
   }

   // if the old node is in the top left
   if (bottomrightdistance <= topleftdistance && bottomrightdistance <= toprightdistance && bottomrightdistance <= bottomleftdistance)
   {
      boundary* subboundary = new boundary;
      subboundary->minX = newbx1;
      subboundary->minY = b->minY;
      subboundary->maxX = b->maxX;
      subboundary->maxY = newby2;

      // the old node then needs to be expanded into its new quarter
      quadtreenode* tl = expandboundary(oldnode, subboundary);
      delete subboundary;

      // create nodes that divide up the new boundary with the dividing lines passing through 
      // the bottom right corner of the old node
      quadtreenode* tr = new quadtreenode(b->maxX, b->minY, newbx2, newby2, capacity, MCP, instancedirectory, resolutionbase, numresolutionlevels);
      quadtreenode* bl = new quadtreenode(newbx1, newby1, b->maxX, b->minY, capacity, MCP, instancedirectory, resolutionbase, numresolutionlevels);
      quadtreenode* br = new quadtreenode(b->maxX, newby1, newbx2, b->minY, capacity, MCP, instancedirectory, resolutionbase, numresolutionlevels);
      delete b;
      return new quadtreenode(newbx1, newby1, newbx2, newby2, capacity, tl, tr, bl, br, MCP, instancedirectory, resolutionbase, numresolutionlevels);
   }

   return NULL;
}





// load a new flight line into the quad tree, nth is the nth points to load
void quadtree::load(lidarpointloader *loader, int nth)
{
   // size of each block of points loaded
   int arraysize = 5000000;
   // add the flightline name flightline num pair to the table
   string tempstring(loader->getfilename());
   flighttable.insert(make_pair(flightlinenum, tempstring));

   int outofboundscounter = 0;
   // get new flight boundary
   boundary *nb = loader->getboundary();
   //int hackcounter = 0;
   

   point *temp = new point[arraysize];

   // resize the array to accomadate new points 
   root = expandboundary(root, nb);
   root->increase_to_minimum_depth(5);

   delete nb;
   int pointcounter;
   ostream &outs = *(errorstream);
   boundary *tempboundary = root->getbound();
   // while there are new points, pull a new block of points from the loader
   // and push them into the tree
   do
   {


      pointcounter = loader->load(arraysize, nth, temp, flightlinenum);
      for (int k = 0; k < pointcounter; k++)
      {
         // try and insert each point
         if (!insert(temp[k]))
         {
            // this block of code appends various information and error messages
            // regarding out of bounds points to the specified error stream
            outofboundscounter++;
            outs << outofboundscounter << ": point out of bounds, diff: ";
            if (temp[k].x < tempboundary->minX)
            {
               outs << "x:" << abs(temp[k].x - tempboundary->minX) << " below minimum ";
            }
            else
               if (temp[k].x > tempboundary->maxX)
            {
               outs << "x:" << abs(temp[k].x - tempboundary->maxX) << " above maximum ";
            }

            if (temp[k].y < tempboundary->minY)
            {
               outs << "y:" << abs(temp[k].y - tempboundary->minY) << " below minimum ";
            }
            else
               if (temp[k].y > tempboundary->maxY)
            {
               outs << "y:" << abs(temp[k].y - tempboundary->maxY) << " above maximum ";
            }
            outs << endl;
         }
      }

   }   while (pointcounter == arraysize);
   flightlinenum++;
   delete[] temp;
   delete tempboundary;

}




// this method loads points from a flightline that fall within an area of intrest
void quadtree::load(lidarpointloader *loader, int nth, double *Xs, double *Ys, int size)
{
   // add the flightline name flightline num pair to the table
   string tempstring(loader->getfilename());
   flighttable.insert(make_pair(flightlinenum, tempstring));

   // get new flight boundary
   boundary *nb = new boundary();
   int outofboundscounter = 0;

   boundary *fb = loader->getboundary();


   if (!AOrec_NAOrec(fb->minX, fb->minY, fb->maxX, fb->maxY, Xs, Ys, size))
   {
      throw outofboundsexception("area of interest falls outside new file");
   }

   delete fb;

   // find the simple bounding box of the new fence
   double largestX,largestY,smallestX,smallestY;
   largestX=Xs[0];smallestX=Xs[0];largestY=Ys[0];smallestY=Ys[0];
   for (int k=1; k<size; k++)
   {
      if(Xs[k] > largestX) {largestX=Xs[k];}
      if(Xs[k] < smallestX) {smallestX=Xs[k];}
      if(Ys[k] > largestY) {largestY=Ys[k];}
      if(Ys[k] < smallestY) {smallestY=Ys[k];}
   }

   nb->minX = smallestX;
   nb->maxX = largestX;
   nb->minY = smallestY;
   nb->maxY = largestY;

   int arraysize = 5000000;
   point *temp = new point[arraysize];
   // expand boundary to cover new points
   root = expandboundary(root, nb);
   root->increase_to_minimum_depth(5);
   ostream &outs = *(errorstream);
   delete nb;
   boundary *tempboundary = root->getbound();
   // while there are new points, pull a new block of points from the loader
   // and push them into the tree
   int pointcount;

   do
   {
      pointcount = loader->load(arraysize, nth, temp, flightlinenum, Xs, Ys, size);
      for (int k = 0; k < pointcount; k++)
      {
         // try and insert each point
         if (!insert(temp[k]))
         {

            // this block of code appends various information and error messages
            // regarding out of bounds points to the specified error stream
            outofboundscounter++;
            outs << outofboundscounter << ": point out of bounds, diff: ";
            if (temp[k].x < tempboundary->minX)
            {
               outs << "x:" << abs(temp[k].x - tempboundary->minX) << " below minimum ";
            }
            else
               if (temp[k].x > tempboundary->maxX)
            {
               outs << "x:" << abs(temp[k].x - tempboundary->maxX) << " above maximum ";
            }

            if (temp[k].y < tempboundary->minY)
            {
               outs << "y:" << abs(temp[k].y - tempboundary->minY) << " below minimum ";
            }
            else
               if (temp[k].y > tempboundary->maxY)
            {
               outs << "y:" << abs(temp[k].y - tempboundary->maxY) << " above maximum ";
            }
            outs << endl;
         }
      }
   }   while (pointcount == arraysize);
   flightlinenum++;
   delete[] temp;
   if (outofboundscounter > 0)
   {
      throw outofboundsexception("points from file outside header boundary, " + outofboundscounter);
   }
}








//deconstructor

quadtree::~quadtree()
{
   delete root;
   delete MCP;
   boost::filesystem::remove_all(instancedirectory);
}

// this is for debugging only usefull for tiny trees (<50)

void quadtree::print()
{
   root->print();
}

// returns true if the quadtree is empty (checks if the root contains
// any additional nodes or any points)

bool quadtree::isEmpty()
{
   return root->isEmpty();

}

// this method takes a point struct, it then attempts to insert it into the
// quadtree.

bool quadtree::insert(point newP)
{
   // check the point falls within the global boundary of the tree
   if (!root->checkbound(newP))
   {
      // abort
      return false;
   }

   // this counter simple keeps track of the total points inserted
   // WARNING : debug code, dosen't take account of deletions
   static int counter;
   counter++;

   // the guess bucket records the last bucket that a point
   // was sucsessfully inserted into. because the lidar records read
   // top to bottom; left to right to left neighbouring points generally
   // fall into the same bucket. checking this bucket first saves time.
   // this pointer keeps track of the location when a full search is
   // needed
   quadtreenode *current;

   // if there is no guessbucket this must be the first run
   if (guessbucket == NULL)
   {
      current = root;
   }
   else
   {
      // first try the guess bucket, if this works we can just return
      if (guessbucket->insert(newP))
      {
         return true;
      }
      else
      {
         // otherwise start at the root of the tree
         current = root;
      }
   }

   // untill a leaf is reached keep picking the child node of the 
   // current node that the new point falls into
   while (!current->isLeaf())
   {
      current = current->pickchild(newP);
   }

   // once the correct leaf is reached attempt to insert
   // NOTE: if the point does not fit the wrong node has 
   // been picked and an error is thrown, this is very bad and means 
   // there is a bug in the insert method
   if (!current->insert(newP))
   {
      throw outofboundsexception("could not insert point into leaf that was chosen, insert broken");
   }

   guessbucket = current;
   return true;
}

bool compare(pointbucket *pb1, pointbucket *pb2)
{
   if (pb1->isincache() && !pb2->isincache())
   {
      return true;
   }
   return false;
}








// small function which is provided to the qsort c function to sort points in buckets

int heightsort(const void * a, const void * b)
{
   double temp = double(((point*) a)->z) - double(((point*) b)->z);
   if (temp < 0)
   {
      return -1;
   }
   if (temp > 0)
   {
      return 1;
   }
   return 0;
}

// ditto

int timesort(const void * a, const void * b)
{
   double temp = double(((point*) a)->time) - double(((point*) b)->time);
   if (temp < 0)
   {
      return -1;
   }
   if (temp > 0)
   {
      return 1;
   }
   return 0;
}

// this function sorts the points within the buckets
// the functionallity of this is contained within the nodes
// which recursivly applie it to all nodes

void quadtree::sort(char v)
{
   if (v == 'H')
   {
      root->sort(heightsort);
   }

   if (v == 'T')
   {
      root->sort(timesort);
   }
}

// this method takes 2 points and a width, the points denote a line which is the center line
// of a rectangle whos width is defined by the width, returns NULL passed two identical points
vector<pointbucket*>* quadtree::advsubset(double *Xs, double *Ys, int size)
{


   // work out from the 2 points and the forumula of the line they describe the four point of
   // the subset rectangle
     
   vector<pointbucket*> *buckets = new vector<pointbucket*>;

   // begin the recursive subsetting of the root node
   root->advsubset(Xs, Ys, size, buckets);
   std::stable_sort(buckets->begin(), buckets->end(), compare);
   return buckets;
}

// returns the boundary of the entire quadtree

boundary* quadtree::getboundary()
{
   if (root == NULL)
   {
      throw nullpointerexception("attempted to get boundary from NULL root");
   }
   return root->getbound();
}

// this method returns the filename of a given flight line number using the
// hashtable built when loading them

string quadtree::getfilename(uint8_t flightlinenum)
{
   flighthash::iterator ity = flighttable.find(flightlinenum);
   if (ity != flighttable.end())
   {
      return ity->second;
   }
   else
   {
      throw outofboundsexception("flightline number does not exist");
   }
}


void quadtree::saveflightline(uint8_t flightlinenum, lidarpointsaver *saver)
{
   point *points = new point[1000000];
   int counter = 0;
   vector<pointbucket*> *buckets;
   boundary *b = root->getbound();

   double *Xs = new double[4];
   double *Ys = new double[4];

   // i should have been shot for this
   Xs[0] = Xs[1] = b->minX;
   Xs[2] = Xs[3] = b->maxX;
   Ys[0] = Ys[3] = b->minY;
   Ys[1] = Ys[2] = b->maxY;

   buckets = advsubset(Xs, Ys, 4);
   pointbucket *current;
   for(unsigned int k=0; k<buckets->size(); k++)
   {
      current = buckets->at(k);
      for(int i=0; i<current->getnumberofpoints(0); i++)
      {
         if(current->getpoint(i, 0).flightline == flightlinenum)
         {
            points[counter] = current->getpoint(i, 0);
            counter++;
            if(counter == 1000000)
            {
               saver->savepoints(counter, points);
               counter = 0;
            }
         }
      }
   }
   saver->savepoints(counter, points);

   delete buckets;
   delete b;
   delete[] points;
}


void quadtree::increasedepth(int i)
{
   root->increasedepth(i);
}


void quadtree::increase_to_minimum_depth(int i)
{
   root->increase_to_minimum_depth(i);
}