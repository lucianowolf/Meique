#ifndef JOBFACTORY_H
#define JOBFACTORY_H

#include <string>
#include <unordered_map>
#include "compileroptions.h"
#include "linkeroptions.h"

class MeiqueScript;
class Node;
class NodeTree;
class Job;

class JobFactory
{
public:
    JobFactory(MeiqueScript& script, NodeTree& nodeTree);
    ~JobFactory();

    void setRoot(Node* root);

    Job* createJob();
    void cacheTargetCompilerOptions(Node* node);
private:
    JobFactory(const JobFactory&) = delete;

    struct Options {
        std::string targetDirectory;
        CompilerOptions compilerOptions;
        LinkerOptions linkerOptions;
    };

    Node* findAGoodNode(Node** target, Node* node);
    Job* createCompilationJob(Node* target, Node* node);
    Job* createTargetJob(Node* target);
    Job* createCustomTargetJob(Node* target, Node* node);
    void fillTargetOptions(Node* node, Options* options);
    void mergeCompilerAndLinkerOptions(Node* node);

    MeiqueScript& m_script;
    NodeTree& m_nodeTree;
    Node* m_root;

    bool m_needToWait;

    typedef std::unordered_map<Node*, Options*> CompilerOptionsMap;
    CompilerOptionsMap m_targetCompilerOptions;
};

#endif
