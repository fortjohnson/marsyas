#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "Collection.h"
#include "MarSystemManager.h"
#include "CommandLineOptions.h"

#include <vector> 

using namespace std;
using namespace Marsyas;

/* global variables for various commandline options */ 
int helpopt_;
int usageopt_;
string wekafname_;
string twekafname_;

string testcollectionfname_;
string predictcollectionfname_;
string mode_;
CommandLineOptions cmd_options_;
string inputdir_;
string outputdir_;
string distancematrix_;
string classifier_;

void 
printUsage(string progName)
{
  MRSDIAG("kea.cpp - printUsage");
  cerr << "Usage : " << progName << " [-m mode -c classifier -id inputdir -od outputdir -w weka file -tw test_weka file -tc test collection file -pr predict collection file] " << endl;
  cerr << endl;
  exit(1);
}

void 
printHelp(string progName)
{
  MRSDIAG("kea.cpp - printHelp");
  cerr << "kea, MARSYAS, Copyright George Tzanetakis " << endl;
  cerr << "--------------------------------------------" << endl;
  cerr << "Kea - machine learning ala Weka " << endl;
  cerr << endl;
  cerr << "Usage : " << progName << endl;
  cerr << "Help Options:" << endl;
  cerr << "-u --usage      : display short usage info" << endl;
  cerr << "-h --help       : display this information " << endl;
  cerr << "-w --wekafname : .arff file for training " << endl;
  cerr << "-tw --testwekafname: .arff file for testing " << endl;
  cerr << "-cl --classifier : classifier to use " << endl;
  cerr << "-m --mode: mode of operation" << endl;
  cerr << "-id --inputdir: input directory" << endl;
  cerr << "-od --outputdir: output directory" << endl;
  cerr << "-dm --distancematrix: distance matrix in MIREX format" << endl;
  cerr << "-tc --testcollectionfname : .mf test collection file " << endl;
  cerr << "-pr --predictcollectionfname : .mf output prediction file " << endl;

  exit(1);
}


void 
distance_matrix_MIREX() 
{
  cout << "Distance matrix calculation using " << wekafname_ << endl;

  wekafname_  = inputdir_ + wekafname_;

  MarSystemManager mng; 

  MarSystem* net = mng.create("Series", "net");
  MarSystem* accum = mng.create("Accumulator", "accum");
  MarSystem* wsrc = mng.create("WekaSource", "wsrc");
  accum->addMarSystem(wsrc);
  accum->updControl("WekaSource/wsrc/mrs_bool/normMaxMin", true);
  accum->updControl("WekaSource/wsrc/mrs_string/filename", wekafname_);
  mrs_natural nInstances = 
    accum->getctrl("WekaSource/wsrc/mrs_natural/nInstances")->to<mrs_natural>();
  accum->updControl("mrs_natural/nTimes", nInstances);

  
  
  MarSystem* dmatrix = mng.create("SelfSimilarityMatrix", "dmatrix");
  dmatrix->addMarSystem(mng.create("Metric", "dmetric"));
  dmatrix->updControl("Metric/dmetric/mrs_string/metric", "euclideanDistance");

  net->addMarSystem(accum);
  net->addMarSystem(dmatrix);

  net->tick();

  ofstream oss;
  oss.open(distancematrix_.c_str());
  
  oss << "Marsyas-kea distance matrix for MIREX 2007 Audio Similarity Exchange " << endl;

  
  // collection simply for naming the entries 
  Collection l;
  l.read(inputdir_ + predictcollectionfname_);
  for (size_t i=1; i <= l.size(); ++i) 
	{
		oss << i << "\t" << l.entry(i-1) << endl;
	}
	

  oss << "Q/R";
  const mrs_realvec& dmx = net->getctrl("mrs_realvec/processedData")->to<mrs_realvec>();
	
  for (int i=1; i <= nInstances; ++i) 
	{
		oss << "\t" << i;
	}
  oss << endl;
	
  for (int i=1; i <= nInstances; ++i) 
	{
		oss << i;
		for (int j=0; j < nInstances; j++)
			oss <<"\t" << dmx(i-1, j);
		oss << endl;
	}
	
  oss << endl;	
}

void 
distance_matrix() 
{
  cout << "Distance matrix calculation using " << wekafname_ << endl;
	
  wekafname_  = inputdir_ + wekafname_;
	
  MarSystemManager mng; 
	
  MarSystem* net = mng.create("Series", "net");
	
  MarSystem* wsrc = mng.create("WekaSource", "wsrc");
  net->addMarSystem(wsrc);
	//!!!: mode control
	net->updControl("WekaSource/wsrc/mrs_string/validationMode", "OutputInstancePair");
	net->updControl("WekaSource/wsrc/mrs_bool/normMaxMin", true);
  net->updControl("WekaSource/wsrc/mrs_string/filename", wekafname_);
	
	
	MarSystem* dmatrix = mng.create("SelfSimilarityMatrix", "dmatrix");
  dmatrix->addMarSystem(mng.create("Metric", "dmetric"));
  dmatrix->updControl("Metric/dmetric/mrs_string/metric", "euclideanDistance");
	//!!!: lmartins: normalization can only be applied when we have all feature vectors in memory...
	//... which is what we are trying to avoid here (having big realvecs in memory)...
  //dmatrix->updControl("mrs_string/normalize", "MinMax");
  net->addMarSystem(dmatrix);
	//!!!: mode control
	net->updControl("SelfSimilarityMatrix/dmatrix/mrs_natural/mode", 1); //FIXME: replace use of enum for strings?
  
	//link controls between WekaSource and SelfSimilarityMatrix
	net->linkControl("SelfSimilarityMatrix/dmatrix/mrs_natural/nInstances", 
									 "WekaSource/wsrc/mrs_natural/nInstances");
	net->linkControl("WekaSource/wsrc/mrs_realvec/instanceIndexes",
									 "SelfSimilarityMatrix/dmatrix/mrs_realvec/instanceIndexes");
	
	ofstream oss;
	oss.open(distancematrix_.c_str());
	oss << "Marsyas-kea distance matrix" << endl;
	
	while(!net->getctrl("SelfSimilarityMatrix/dmatrix/mrs_bool/done")->to<bool>())
	{
		const mrs_realvec& idxs = net->getctrl("SelfSimilarityMatrix/dmatrix/mrs_realvec/instanceIndexes")->to<mrs_realvec>();
		oss << "(" << mrs_natural(idxs(0)) << "," << mrs_natural(idxs(1)) << ") = ";
		
		net->tick();
				
		const mrs_realvec& value = net->getctrl("mrs_realvec/processedData")->to<mrs_realvec>();
		oss << value(0) << endl;
	}
	
  oss << endl;	
}


void 
pca() 
{
  cout << "Principal Component Analysis of .arff file" << endl;
  if (wekafname_ == EMPTYSTRING) 
    {
      cout << "Weka .arff file not specified" << endl;
      return;
    }

  wekafname_  = inputdir_ + wekafname_;

  cout << "PCA using .arff file: " << wekafname_ << endl;

  MarSystemManager mng;

  MarSystem* net = mng.create("Series", "net");
  MarSystem* accum = mng.create("Accumulator", "accum");
  MarSystem* wsrc = mng.create("WekaSource", "wsrc");
  accum->addMarSystem(wsrc);
  accum->updControl("WekaSource/wsrc/mrs_string/filename", wekafname_);
  mrs_natural nInstances = 
    accum->getctrl("WekaSource/wsrc/mrs_natural/nInstances")->to<mrs_natural>();
  cout << "nInstances = " << nInstances << endl;
  accum->updControl("mrs_natural/nTimes", nInstances);

  net->addMarSystem(accum);
  net->addMarSystem(mng.create("PCA", "pca"));
  net->addMarSystem(mng.create("NormMaxMin", "norm"));
  net->addMarSystem(mng.create("WekaSink", "wsink"));

  net->updControl("PCA/pca/mrs_natural/npc", 2);
  net->updControl("NormMaxMin/norm/mrs_natural/ignoreLast", 1);
  net->updControl("NormMaxMin/norm/mrs_string/mode", "twopass");
  net->updControl("NormMaxMin/norm/mrs_real/lower", 0.0);
  net->updControl("NormMaxMin/norm/mrs_real/upper", 11.0);
  
  net->updControl("WekaSink/wsink/mrs_natural/nLabels", 
	       net->getctrl("Accumulator/accum/WekaSource/wsrc/mrs_natural/nClasses"));
  net->updControl("WekaSink/wsink/mrs_string/labelNames", net->getctrl("Accumulator/accum/WekaSource/wsrc/mrs_string/classNames"));
  net->updControl("WekaSink/wsink/mrs_string/filename", "pca_out.arff");

  net->tick();

  // the output of the PCA 
 const mrs_realvec& pca_transformed_data = net->getctrl("mrs_realvec/processedData")->to<mrs_realvec>();

 cout << "Output transformed features using PCA" << endl;

 string classNames = net->getctrl("Accumulator/accum/WekaSource/wsrc/mrs_string/classNames")->to<mrs_string>(); 
 vector<string> labelNames;
 
 labelNames.clear();
 
 for (int i = 0; i < net->getctrl("Accumulator/accum/WekaSource/wsrc/mrs_natural/nClasses")->to<mrs_natural>(); ++i)
 {
	 string labelName;
	 string temp;
	 
	 labelName = classNames.substr(0, classNames.find(","));
	 temp = classNames.substr(classNames.find(",")+1, classNames.length());
	 classNames = temp;
	 labelNames.push_back(labelName);
 }
 
 
 cout << "12" << endl;
 cout << "12" << endl;
 
 for (int t=0; t < pca_transformed_data.getCols(); t++) 
   {

	   cout << (int)pca_transformed_data(0,t) * 12 + (int)pca_transformed_data(1,t) << ",";
	   cout << labelNames[(int)pca_transformed_data(2,t)];
	   cout << endl;
	   
	   // cout << (int)pca_transformed_data(0,t) << "\t";
	   // cout << (int)pca_transformed_data(1,t) << "\t";
	   // cout << (int)pca_transformed_data(2,t) << "\t";
     // cout << pca_transformed_data(3,t) << "\t";
	   // cout << endl;
   }

  

  

}

void 
train_and_predict() 
{
  if (wekafname_ == EMPTYSTRING) 
  {
		cout << "Weka .arff file not specified" << endl;
      return;
    }
  
  wekafname_  = inputdir_ + wekafname_;

  cout << "Training classifier using .arff file: " << wekafname_ << endl;
  cout << "Classifier type : " << classifier_ << endl;	
  cout << "Predicting classes for .arff file: " << twekafname_ << endl;
  


  MarSystemManager mng;

  ////////////////////////////////////////////////////////////
  //
  // The network that we will use to train and predict
  //
  MarSystem* net = mng.create("Series", "series");

  ////////////////////////////////////////////////////////////
  //
  // The WekaSource we read the train and test .arf files into
  //
  net->addMarSystem(mng.create("WekaSource", "wsrc"));

  ////////////////////////////////////////////////////////////
  //
  // The classifier
  //
  MarSystem* classifier = mng.create("Classifier", "cl");
  net->addMarSystem(classifier);

  ////////////////////////////////////////////////////////////
  //
  // Which classifier function to use
  //
  string classifier_ = "SVM";
  if (classifier_ == "GS")
	net->updControl("Classifier/cl/mrs_string/enableChild", "GaussianClassifier/gaussiancl");
  if (classifier_ == "ZEROR") 
	net->updControl("Classifier/cl/mrs_string/enableChild", "ZeroRClassifier/zerorcl");    
  if (classifier_ == "SVM")   
    net->updControl("Classifier/cl/mrs_string/enableChild", "SVMClassifier/svmcl");    

  ////////////////////////////////////////////////////////////
  //
  // The training file we are feeding into the WekaSource
  //
  net->updControl("WekaSource/wsrc/mrs_string/filename", wekafname_);
  net->updControl("mrs_natural/inSamples", 1);

  ////////////////////////////////////////////////////////////
  //
  // Set the classes of the Summary and Classifier to be
  // the same as the WekaSource
  //
  net->updControl("Classifier/cl/mrs_natural/nClasses", net->getctrl("WekaSource/wsrc/mrs_natural/nClasses"));
  net->updControl("Classifier/cl/mrs_string/mode", "train");  

  ////////////////////////////////////////////////////////////
  //
  // Tick over the training WekaSource until all lines in the
  // training file have been read.
  //


  while (!net->getctrl("WekaSource/wsrc/mrs_bool/done")->to<mrs_bool>()) {
	string mode = net->getctrl("WekaSource/wsrc/mrs_string/mode")->to<mrs_string>();
  	net->tick();
	net->updControl("Classifier/cl/mrs_string/mode", mode);
  }

  cout << "Done training " << endl;
  

//     cout << "------------------------------" << endl;
//     cout << "Class names" << endl;
//     cout << net->getctrl("WekaSource/wsrc/mrs_string/classNames") << endl;
//     cout << "------------------------------\n" << endl;



   vector<string> classNames;
   string s = net->getctrl("WekaSource/wsrc/mrs_string/classNames")->to<mrs_string>();
   char *str = (char *)s.c_str();
   char * pch;
   pch = strtok (str,",");
   classNames.push_back(pch);
   while (pch != NULL) {
 	pch = strtok (NULL, ",");
 	if (pch != NULL)
 	  classNames.push_back(pch);
   }


  ////////////////////////////////////////////////////////////
  //
  // Predict the classes of the test data
  //
  net->updControl("WekaSource/wsrc/mrs_string/filename", twekafname_);
  net->updControl("Classifier/cl/mrs_string/mode", "predict");  
  ////////////////////////////////////////////////////////////
  //
  // Tick over the test WekaSource until all lines in the
  // test file have been read.
  //

  
  ofstream prout;
  prout.open(predictcollectionfname_.c_str());

  

  realvec data;
  while (!net->getctrl("WekaSource/wsrc/mrs_bool/done")->to<mrs_bool>()) {
   	net->tick();
   	data = net->getctrl("mrs_realvec/processedData")->to<mrs_realvec>();
	cout << net->getctrl("WekaSource/wsrc/mrs_string/currentFilename")->to<mrs_string>() << "\t";
  	cout << classNames[(int)data(0,0)] << endl;
	prout << net->getctrl("WekaSource/wsrc/mrs_string/currentFilename")->to<mrs_string>() << "\t";
  	prout << classNames[(int)data(0,0)] << endl;

	//	cout << data(0,0) << endl;
  }

  
  prout.close();
  
  // cout << "DONE" << endl;

  // sness - hmm, I really should be able to delete net, but I get a 
  // coredump when I do.  Maybe I need to destroy something else first?
  //  delete net;





}



void 
train()
{
  if (wekafname_ == EMPTYSTRING) 
    {
      cout << "Weka .arff file not specified" << endl;
      return;
    }

  wekafname_  = inputdir_ + wekafname_;

  cout << "Training classifier using .arff file: " << wekafname_ << endl;
  cout << "Classifier type : " << classifier_ << endl;

  
  MarSystemManager mng;
  
  MarSystem* net;
  net = mng.create("Series", "net");
  net->addMarSystem(mng.create("WekaSource", "wsrc"));
  net->addMarSystem(mng.create("Classifier", "cl"));
  net->addMarSystem(mng.create("ClassificationReport", "summary"));

  if (classifier_ == "GS")
    net->updControl("Classifier/cl/mrs_string/enableChild", "GaussianClassifier/gaussiancl");
  if (classifier_ == "ZEROR") 
    net->updControl("Classifier/cl/mrs_string/enableChild", "ZeroRClassifier/zerorcl");    
  if (classifier_ == "SVM")   
    net->updControl("Classifier/cl/mrs_string/enableChild", "SVMClassifier/svmcl");    
  // net->updControl("WekaSource/wsrc/mrs_string/attributesToInclude", "1,2,3");
  

  // net->updControl("WekaSource/wsrc/mrs_string/validationMode", "PercentageSplit,50%");
  net->updControl("WekaSource/wsrc/mrs_string/validationMode", "kFold,NS,10");
  // net->updControl("WekaSource/wsrc/mrs_string/validationMode", "UseTestSet,lg.arff");
  net->updControl("WekaSource/wsrc/mrs_string/filename", wekafname_);
  net->updControl("mrs_natural/inSamples", 1);

  net->updControl("ClassificationReport/summary/mrs_natural/nClasses", net->getctrl("WekaSource/wsrc/mrs_natural/nClasses"));
  net->updControl("ClassificationReport/summary/mrs_string/classNames", 
	       net->getctrl("WekaSource/wsrc/mrs_string/classNames"));
  
  net->updControl("Classifier/cl/mrs_natural/nClasses", net->getctrl("WekaSource/wsrc/mrs_natural/nClasses"));
  net->linkControl("Classifier/cl/mrs_string/mode", "ClassificationReport/summary/mrs_string/mode");  

  int i = 0;
  while(net->getctrl("WekaSource/wsrc/mrs_bool/done")->to<mrs_bool>() == false)
    {
      net->tick();
      string mode = net->getctrl("WekaSource/wsrc/mrs_string/mode")->to<mrs_string>();
      net->updControl("Classifier/cl/mrs_string/mode", mode);
      ++i;
    }

  net->updControl("Classifier/cl/mrs_string/mode", "predict");
  net->updControl("ClassificationReport/summary/mrs_bool/done", true);
  net->tick();
}

//
// 
//
void tags() {

  MarSystemManager mng;

  ////////////////////////////////////////////////////////////
  //
  // The network that we will use to train and predict
  //
  MarSystem* net = mng.create("Series", "series");

  ////////////////////////////////////////////////////////////
  //
  // The WekaSource we read the train and test .arf files into
  //
  net->addMarSystem(mng.create("WekaSource", "wsrc"));

  ////////////////////////////////////////////////////////////
  //
  // The classifier
  //
  MarSystem* classifier = mng.create("Classifier", "cl");
  net->addMarSystem(classifier);
  net->addMarSystem(mng.create("Gain/gain"));

  ////////////////////////////////////////////////////////////
  //
  // Which classifier function to use
  //
  string classifier_ = "SVM";
  if (classifier_ == "GS")
	net->updControl("Classifier/cl/mrs_string/enableChild", "GaussianClassifier/gaussiancl");
  if (classifier_ == "ZEROR") 
	net->updControl("Classifier/cl/mrs_string/enableChild", "ZeroRClassifier/zerorcl");    
  if (classifier_ == "SVM")   
    net->updControl("Classifier/cl/mrs_string/enableChild", "SVMClassifier/svmcl");    

  ////////////////////////////////////////////////////////////
  //
  // The training file we are feeding into the WekaSource
  //
  
  net->updControl("WekaSource/wsrc/mrs_string/filename", inputdir_ + wekafname_);
  net->updControl("mrs_natural/inSamples", 1);

  ////////////////////////////////////////////////////////////
  //
  // Set the classes of the ClassificationReport and Classifier to be
  // the same as the WekaSource
  //
  net->updControl("Classifier/cl/mrs_natural/nClasses", net->getctrl("WekaSource/wsrc/mrs_natural/nClasses"));
  net->linkControl("Classifier/cl/mrs_string/mode", "mrs_string/train");  

  ////////////////////////////////////////////////////////////
  //
  // Tick over the training WekaSource until all lines in the
  // training file have been read.
  //
  int counter = 0;

  cout << "Reading features" << endl;
  
  while (!net->getctrl("WekaSource/wsrc/mrs_bool/done")->to<mrs_bool>()) {
	string mode = net->getctrl("WekaSource/wsrc/mrs_string/mode")->to<mrs_string>();
  	net->tick();
	net->updControl("Classifier/cl/mrs_string/mode", mode);
	counter++;
  }

  cout << "Training" << endl;
  
  mrs_natural nLabels = net->getctrl("WekaSource/wsrc/mrs_natural/nClasses")->to<mrs_natural>();
  mrs_string labelNames = net->getctrl("WekaSource/wsrc/mrs_string/classNames")->to<mrs_string>();

  vector<string> classNames;
  string s = net->getctrl("WekaSource/wsrc/mrs_string/classNames")->to<mrs_string>();


  for (int i=0; i < nLabels; ++i) 
    {
      string className;
      string temp;
      className = s.substr(0, s.find(","));
      temp = s.substr(s.find(",") + 1, s.length());
      s = temp;
      classNames.push_back(className);
    }




  ////////////////////////////////////////////////////////////
  //
  // Predict the classes of the test data
  //

  
  net->updControl("WekaSource/wsrc/mrs_string/filename", inputdir_ + testcollectionfname_);
  net->updControl("Classifier/cl/mrs_string/mode", "predict");  
  
  // The output prediction file
  ofstream prout;
  prout.open(predictcollectionfname_.c_str());

  ////////////////////////////////////////////////////////////
  //
  // Tick over the test WekaSource until all lines in the
  // test file have been read.
  //
  realvec data;
  mrs_string currentlyPlaying;
  vector<string> previouslySeenFilenames;
  bool seen;
  realvec wsourcedata;


  


  
  MarSystem* wsink = mng.create("WekaSink/wsink");
  
  wsink->updControl("mrs_natural/inSamples", 1);
  wsink->updControl("mrs_natural/inObservations", nLabels+1);  
  wsink->updControl("mrs_natural/nLabels", nLabels);
  wsink->updControl("mrs_string/labelNames", labelNames);  
  wsink->updControl("mrs_string/inObsNames", labelNames);
  wsink->updControl("mrs_string/filename", outputdir_ + "stacked_" + testcollectionfname_);




  cout << "Starting prediction" << endl;

  mrs_realvec wsinkout;
  mrs_natural label;
  mrs_realvec probs;
  
  wsinkout.create(nLabels+1,1);

  cout << "Starting Prediction for Testing Collection" << endl;
  
  counter = 0;
  
  while (!net->getctrl("WekaSource/wsrc/mrs_bool/done")->to<mrs_bool>()) 
    {
		counter++;
		
      net->tick();
      wsourcedata = net->getctrl("WekaSource/wsrc/mrs_realvec/processedData")->to<mrs_realvec>();
      data = net->getctrl("mrs_realvec/processedData")->to<mrs_realvec>();
      
      currentlyPlaying = net->getctrl("WekaSource/wsrc/mrs_string/currentFilename")->to<mrs_string>();
      
      seen = false;
      
      for (size_t i=0; i<previouslySeenFilenames.size(); ++i) {
	if (currentlyPlaying == previouslySeenFilenames[i]) {
	  seen = true;
	  break;
	}
      }
      
      if (seen == false) {
	probs = net->getctrl("Classifier/cl/mrs_realvec/processedData")->to<mrs_realvec>();
	for (mrs_natural j=0; j < probs.getSize()-2; j++) 
	  wsinkout(j,0) = probs(2+j);
	
	
	for (mrs_natural i=0; i < probs.getSize()-2; ++i) {
	  cout << currentlyPlaying << "\t" << classNames[i] << "\t" << probs(2+i) << endl;
	  prout << currentlyPlaying << "\t" << classNames[i] << "\t" << probs(2+i) << endl;

	  
	}

	wsinkout(probs.getSize()-2,0) = probs(0);
	wsink->updControl("mrs_string/currentlyPlaying", currentlyPlaying);
	wsink->process(wsinkout,wsinkout);	  
	
	previouslySeenFilenames.push_back(currentlyPlaying);
      } 
      

      
      
    }

  ////////////////////////////////////////////////////////////
  //
  // Tick over the test WekaSource until all lines in the
  // test file have been read.
  //
  realvec data2;
  mrs_string currentlyPlaying2;
  vector<string> previouslySeenFilenames2;
  realvec wsourcedata2;


  


  MarSystem* wsink2 = mng.create("WekaSink/wsink2");
  
  wsink2->updControl("mrs_natural/inSamples", 1);
  wsink2->updControl("mrs_natural/inObservations", nLabels+1);  
  wsink2->updControl("mrs_natural/nLabels", nLabels);
  wsink2->updControl("mrs_string/labelNames", labelNames);  
  wsink2->updControl("mrs_string/inObsNames", labelNames);
  wsink2->updControl("mrs_string/filename", outputdir_ + "stacked_" + wekafname_);
  

  cout << "Starting prediction for training collection (for stacked generalization)" << endl;

  mrs_realvec wsinkout2;
  mrs_realvec probs2;
  wsinkout2.create(nLabels+1,1);

  net->updControl("WekaSource/wsrc/mrs_string/filename", inputdir_ + wekafname_); 
  
  while (!net->getctrl("WekaSource/wsrc/mrs_bool/done")->to<mrs_bool>()) {
    net->tick();
    currentlyPlaying2 = net->getctrl("WekaSource/wsrc/mrs_string/currentFilename")->to<mrs_string>();
    wsourcedata = net->getctrl("WekaSource/wsrc/mrs_realvec/processedData")->to<mrs_realvec>();
    label = wsourcedata(wsourcedata.getSize()-1,0);
    seen = false;
    
    for (mrs_natural i=0; i<(mrs_natural)previouslySeenFilenames2.size(); ++i) {
      if (currentlyPlaying2 == previouslySeenFilenames2[i]) {
	seen = true;
	break;
      }
    }
    
    probs2 = net->getctrl("Classifier/cl/mrs_realvec/processedData")->to<mrs_realvec>();    
    if (seen == false) {
      for (mrs_natural j=0; j < probs2.getSize()-2; j++) 
	wsinkout2(j,0) = probs2(2+j);
      
      for (mrs_natural i=0; i < probs2.getSize()-2; ++i) {
	cout << currentlyPlaying2 << "\t" << classNames[i] << "\t" << probs2(2+i) << endl;
      }
      previouslySeenFilenames2.push_back(currentlyPlaying2);
    } 
    
    wsinkout2(probs2.getSize()-2,0) = label;    
    wsink2->updControl("mrs_string/currentlyPlaying", currentlyPlaying2);
    wsink2->process(wsinkout2,wsinkout2);
  }
  
  cout << "DONE" << endl;

}


void 
initOptions()
{
  cmd_options_.addBoolOption("help", "h", false);
  cmd_options_.addBoolOption("usage", "u", false);
  cmd_options_.addStringOption("wekafname", "w", EMPTYSTRING);
  cmd_options_.addStringOption("testwekafname", "tw", EMPTYSTRING);
  cmd_options_.addStringOption("testcollectionfname", "tc", EMPTYSTRING);
  cmd_options_.addStringOption("predictcollectionfname", "pr", EMPTYSTRING);
  cmd_options_.addStringOption("mode", "m", "train");
  cmd_options_.addStringOption("inputdir", "id", "");
  cmd_options_.addStringOption("outputdir", "od", "");
  cmd_options_.addStringOption("distancematrix", "dm", "dm.txt");
  cmd_options_.addStringOption("classifier", "cl", "SVM");
}


void 
loadOptions()
{
  helpopt_ = cmd_options_.getBoolOption("help");
  usageopt_ = cmd_options_.getBoolOption("usage");
  wekafname_ = cmd_options_.getStringOption("wekafname");
  twekafname_ = cmd_options_.getStringOption("testwekafname");
  testcollectionfname_ = cmd_options_.getStringOption("testcollectionfname");
  predictcollectionfname_ = cmd_options_.getStringOption("predictcollectionfname");
  mode_ = cmd_options_.getStringOption("mode");
  inputdir_ = cmd_options_.getStringOption("inputdir");
  outputdir_ = cmd_options_.getStringOption("outputdir");
  distancematrix_ = cmd_options_.getStringOption("distancematrix");
  classifier_ = cmd_options_.getStringOption("classifier");
}


int
main(int argc, const char **argv)
{
  MRSDIAG("train.cpp - main");

  cout << "Kea - Machine Learning in Marsyas ala Weka" << endl;

  string progName = argv[0];  
  if (argc == 1)
    printUsage(progName);

  // handling of command-line options 
  initOptions();
  cmd_options_.readOptions(argc, argv);
  loadOptions();
  
  vector<string> soundfiles = cmd_options_.getRemaining();
  if (helpopt_) 
    printHelp(progName);
  
  if (usageopt_)
    printUsage(progName);

  cout << "Mode = " << mode_ << endl;

  if (mode_ == "train") 
    train();
  if (mode_ == "distance_matrix_MIREX")
	  distance_matrix_MIREX();
  if (mode_ == "distance_matrix") 
	  distance_matrix();
  if (mode_ == "pca")
	  pca();
  if (mode_ == "tags")
	  tags();
  if (mode_ == "train_predict") 
	  train_and_predict();
  
  
  exit(0);
}






