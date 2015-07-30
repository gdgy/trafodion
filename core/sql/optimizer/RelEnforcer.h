/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@
**********************************************************************/
#ifndef RELENFORCER_H
#define RELENFORCER_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         RelEnforcer.h
 * Description:  Enforcers for physical properties
 * Created:      3/6/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#include "RelExpr.h"
#include "PartFunc.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class Sort;
class Exchange;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
class CostScalar;
class SimpleCostVector;
class CorrName;
class SearchKey;

///////////////////////////////////////////////////////////////////////////
// SortLogical node is a shortlived node to keep track of an 'order by'
// clause at the top level of a view.
// It is created during bind phase view expansion and is eliminated during
// the normalization phase.
// See method BindWA::bindView, RelRoot::transformNode 
// and SortLogical::normalizeNode for details.
///////////////////////////////////////////////////////////////////////////
class SortLogical : public RelExpr
{
public:
  SortLogical(RelExpr *child,
	      const ValueIdList &sortKey,
	      CollHeap *oHeap = CmpCommon::statementHeap()) :
       RelExpr(REL_SORT_LOGICAL,child, NULL, oHeap),
       sortKey_(sortKey)
  {}

  // Each operator supports a (virtual) method for performing
  // predicate pushdown and computing a "minimal" set of
  // characteristic input and characteristic output values.
  virtual RelExpr * normalizeNode(NormWA & normWARef);

  // accessor functions
  inline ValueIdList &getSortKey() { return sortKey_; }

  // get the degree of this node (it is a unary op).
  virtual Int32 getArity() const { return 1; };

private:

  // the sort key list
  ValueIdList sortKey_;       // the order of sort keys is relevant
};

// -----------------------------------------------------------------------
// The Sort operator returns the same rows as its child, but in a given
// sort order. It is generated by the sort enforcer rule in case where
// its parent node requires a specific sort order or an arrangement of
// columns. It can also take an ordered stream of rows from the child
// and extend an existing ordering key of the child
// (alreadySortedColumns_) by additional expressions.
// -----------------------------------------------------------------------
class Sort : public RelExpr
{
public:

  // constructors
  Sort(RelExpr *child,
	      CollHeap *oHeap = CmpCommon::statementHeap()) :
       RelExpr(REL_SORT,child,NULL,oHeap),
       sortNRows_(FALSE), 
       collectNFErrors_(FALSE),
       forcedHalloweenProtection_(FALSE),
       checkAccessToSelfRefTable_(FALSE)
	 {}
  Sort(RelExpr *child,
	      const ValueIdList &sortKey,
	      CollHeap *oHeap = CmpCommon::statementHeap()) :
       RelExpr(REL_SORT,child, NULL, oHeap),
       sortKey_(sortKey),
       sortNRows_(FALSE), 
       collectNFErrors_(FALSE),
       forcedHalloweenProtection_(FALSE),
       checkAccessToSelfRefTable_(FALSE)
	 {}
  inline Sort(RelExpr *child,
	      const ValueIdSet &arrangedCols) :
       RelExpr(REL_SORT,child),
       arrangedCols_(arrangedCols),
       sortNRows_(FALSE), 
    forcedHalloweenProtection_(FALSE),
    checkAccessToSelfRefTable_(FALSE),
    collectNFErrors_(FALSE)
	 {}

  virtual ~Sort();

  // get the degree of this node (it is a unary op).
  virtual Int32 getArity() const;

  virtual RelExpr * preCodeGen(Generator * generator,
			       const ValueIdSet & externalInputs,
			       ValueIdSet &pulledNewInputs);
  virtual short codeGen(Generator*);
  virtual void needSortedNRows(NABoolean val);
  NABoolean &sortNRows() {return sortNRows_;}

  virtual HashValue topHash();
  virtual NABoolean duplicateMatch(const RelExpr & other) const;
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = NULL);

  virtual NABoolean isLogical() const;
  virtual NABoolean isPhysical() const;

  // Cascades-related functions
  virtual CostMethod* costMethod() const;
  virtual Context* createContextForAChild(Context* myContext,
                     PlanWorkSpace* pws,
                     Lng32& childindex);

  virtual PhysicalProperty* synthPhysicalProperty(const Context* myContext,
                                                  const Lng32    planNumber,
                                                  PlanWorkSpace  *pws);

  void synthPartialSortKeyFromChild(const Context*);
  virtual void produceFinalSortKey();

  // get a printable string that identifies the operator
  virtual const NAString getText() const;

  // add all the expressions that are local to this
  // node to an existing list of expressions (used by GUI tool)
  virtual void addLocalExpr(LIST(ExprNode *) &xlist,
			    LIST(NAString) &llist) const;

  // accessor functions
  inline ValueIdList &getSortKey()                    { return sortKey_; }
  inline ValueIdList &getPartialSortKeyFromChild() 
                   { return PartialSortKeyFromChild_;} 
  inline ValueIdList &getPrefixSortKey()
                   { return PartialSortKeyFromChild_;}

  inline ValueIdSet &getArrangedCols()           { return arrangedCols_; }

  // The method gets refined since Sort may be a BMO depending on its inputs.
  virtual NABoolean isBigMemoryOperator(const PlanWorkSpace* pws,
                                        const Lng32 planNumber);

  virtual CostScalar getEstimatedRunTimeMemoryUsage(NABoolean perCPU);
  virtual double getEstimatedRunTimeMemoryUsage(ComTdb * tdb);

  virtual double getEstimatedRunTimeOverflowSize(double memoryQuotaMB);

  virtual PlanPriority computeOperatorPriority
    (const Context* context,
     PlanWorkSpace *pws=NULL,
     Lng32 planNumber=0);

  inline void markAsHalloweenProtection() { forcedHalloweenProtection_= TRUE; }
  inline void setCollectNFErrors(NABoolean cf = TRUE) {collectNFErrors_ = cf;}
  inline NABoolean collectNFErrors() {return collectNFErrors_;}

  inline void doCheckAccessToSelfRefTable() { checkAccessToSelfRefTable_ = TRUE; }

  ExplainTuple *addSpecificExplainInfo(ExplainTupleMaster *explainTuple,
					      ComTdb * tdb,
					      Generator *generator);

  virtual NABoolean sortFromTop() { return FALSE; }
protected:

  // the sort key list
  ValueIdList sortKey_;       // the order of sort keys is relevant
  ValueIdSet  arrangedCols_;  // a "fuzzy" sort key, actual order can
                              // be determined later

  // set to TRUE, if top N sorted rows are needed.
  // See method needSortedNRows().
  NABoolean sortNRows_;

  // Introduced specifically to protect against halloween problem.
  // If a sort was chosen by the optimizer for any other reason,
  // this flag will be false.  It is set to true by NestJoin::preCodeGen.
  NABoolean forcedHalloweenProtection_;

  // A "halloween sort" needs to ensure that if it is parallel, but executes
  // in the same ESP as the generic update's TSJ flow node, then the Sort
  // will block until all scans are finished.  This logic is enforced in the 
  // PreCodeGen phase.  This next flag helps.
  NABoolean checkAccessToSelfRefTable_;

  // we keep track of order of input stream to see if it could be used
  // by order and/or arrangement requirements
  ValueIdList PartialSortKeyFromChild_;

  NABoolean collectNFErrors_;

  short generateTdb(Generator * generator,
                    ComTdb * child_tdb,
                    ex_expr * sortKeyExpr,
                    ex_expr * sortRecExpr,
                    ULng32 sortKeyLen,
                    ULng32 sortRecLen,
                    ULng32 sortPrefixKeyLen,
                    ex_cri_desc * given_desc,
                    ex_cri_desc * returned_desc,
                    ex_cri_desc * work_cri_desc,
                    Lng32 saveNumEsps,
                    ExplainTuple *childExplainTuple,
                    NABoolean resizeCifRecord,
                    NABoolean considerBufferDefrag,
                    NABoolean operatorCIF = FALSE);

  //determine internal format
  /*
  virtual ExpTupleDesc::TupleDataFormat determineInternalFormat( const ValueIdList & valIdList,
                                                                    RelExpr * relExpr,
                                                                    NABoolean & resizeCifRecord,
                                                                    Generator * generator);*/

  ExpTupleDesc::TupleDataFormat determineInternalFormat( const ValueIdList & valIdList,
                                                         RelExpr * relExpr,
                                                         NABoolean & resizeCifRecord,
                                                         Generator * generator,
                                                         NABoolean bmo_affinity ,
                                                         NABoolean & considerBufferDefrag);

};

class SortFromTop : public Sort
{
public:

  // constructors
  SortFromTop(RelExpr *child,
	      CollHeap *oHeap = CmpCommon::statementHeap()) :
       Sort(child, oHeap)
  {}

  virtual RelExpr * preCodeGen(Generator * generator,
			       const ValueIdSet & externalInputs,
			       ValueIdSet &pulledNewInputs);
  virtual short codeGen(Generator*);

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = NULL);

  // get a printable string that identifies the operator
  virtual const NAString getText() const;

  virtual NABoolean sortFromTop() { return TRUE; }

  ValueIdList &getSortRecExpr() { return sortRecExpr_; }

private:
  // list of values to be used to create the input row to sort
  ValueIdList sortRecExpr_;       
};

// 
// preCodeGen (pcg) phase esp fragment. This class is used by over
// parallelization correction logic and is not related to FragmentDir.
//
class pcgEspFragment {

public:

  pcgEspFragment(CollHeap* heap) :
     childEsps_(heap), valid_(FALSE),totalRows_(csZero), newDoP_(0), 
     commonDoP_(0) {};

  ~pcgEspFragment() {};

  void addChild(Exchange* esp);

  void accumulateRows(CostScalar x) { totalRows_ += x; } ;

  CostScalar getTotaleRows() { return totalRows_; } ;

  NABoolean isValid() const { return valid_; };

  // reduce the dop at this fragment. 
  NABoolean tryToReduceDoP(); 

  // invalidate DoP for my fragment and my child fragments
  void invalidate();     

  // adjust the dop 
  void adjustDoP(Lng32 newDop);

  Lng32 getNewDop() { return newDoP_;}

protected:
  void setValid(NABoolean x) { valid_ = x; };

protected:
  NAArray<Exchange*> childEsps_;

  CostScalar totalRows_;  // total rows to be rocessed;

  Lng32 newDoP_;         // new proposed dop for the fragment 

  Lng32 commonDoP_;      // a common dop fro the 1st child esp with
                         // non-broadcast parf func, for other 
                         // non-broadcast child esps to match

  NABoolean valid_;      // whether this fragment is a candidate for
                         // over parallelization correction.
};


// -----------------------------------------------------------------------
// class Exchange
//
// An Exchange is used for realizing the parallel execution of query
// plan fragments (ESP parallelism) as well as for asynchronous
// communication between an ESP and the disk server process (DP2).
//
// The optimizer introduces an Exchange operator in the dataflow tree
// in order to send data from one process to another, either to change
// the plan execution location (DP2, ESP, Master), or to redistribute
// the workload to multiple processes.
//
// A process boundary to DP2 (DP2 exchange) is introduced when data or
// control needs to be returned from DP2. There are two flavors of
// DP2 exchanges, as described below.
//
// An ESP exchange introduces an Executor Server Process, or ESP when
// an operator in the query execution plan is expected to consume
// memory very heavily, or when a tree of operators implements a
// cpu-intensive operation that is likely to saturate the cpu, or
// asynchronous access to partitioned data can improve the throughput
// of an operator, or operator trees. It encapsulates parallelism as
// described by Goetz Graefe for the Volcano prototype.  Jim Gray
// calls the communication pattern created by the Exchange operator a
// "river".
//
// The Exchange that is implemented for the SQL/ARK optimizer is just
// another dataflow operator. It is represented using the notation
// Exchange(n:m), where m is the number of data streams it receives as
// its internal dataflow inputs and n is the number of data streams it
// produces as its output. In Cascades jargon, it is purely a physical
// operator and is a partition and location enforcer. We consistently
// use the term "stream" in preference over the term "partition",
// while referring to the input values received by an Exchange as well
// as the output values produced by it, because we view them to be
// orthognal to each other. For historical reasons, the PartitioningFunction
// classes use the term "partition" which is equivalent to the term
// "stream" in this context.
//
// In an attempt to disambiguate the terms input and output, we
// introduce two new terms "top" and "bottom" to classify the
// dataflow. We call the root of the query tree its "top" and
// the leaves of the query tree its "bottom":
//
//                   |   n "top" data stream(s) for the Exchange
//                   |
//                   X   an Exchange operator n:m
//                   |
//                   |   m "bottom" data stream(s) for the Exchange
//
//         Fig 1. A sample of dataflow through the Exchange
//
// After optimization, an Exchange node contains a descriptor that
// describes the partitioning characteristics of its top data streams,
// which include the number of data streams, the partitioning key
// columns as well as the partitioning function that will be used for
// creating them. The descriptor is implemented by the class
// PartitioningFunction.  The Exchange also contains another
// PartitioningFunction for describing its bottom data streams.
//
// -----------------------------------------------------------------------

class Exchange : public RelExpr
{
public:

  // constructor
  Exchange(RelExpr *childSubtree,
	   CollHeap *oHeap = CmpCommon::statementHeap())
    : RelExpr(REL_EXCHANGE,childSubtree, NULL, oHeap),
      topPartFunc_(NULL),
      bottomPartFunc_(NULL),
      bottomLocIsSet_(FALSE),
      isRedundant_(FALSE),
      skipRedundancyCheck_(FALSE),
      indexDesc_(NULL),
      partSearchKey_(NULL),
      DP2TransactionIndicator_(FALSE),
      numBMOs_(0),
      BMOsMemoryUsage_(0),
      isOverReverseScan_(FALSE),
      extractSelectList_(NULL),
      isExtractProducer_(FALSE),
      isExtractConsumer_(FALSE),
      upMessageBufferLength_(0),
      downMessageBufferLength_(0),
      forcedHalloweenProtection_(FALSE),
      halloweenSortIsMyChild_(FALSE),
      hash2RepartitioningWithSameKey_(FALSE),
      isAnESPAccess_(FALSE),
      pcgEspFragment_(oHeap)
   {}

  virtual ~Exchange();

  // get the degree of this node (it is a unary op).
  virtual Int32 getArity() const;

  // ---------------------------------------------------------------------
  // The Exchange operator that is seen by the optimizer denotes a
  // group of one or more operators, which are distinct from it, that
  // realize interprocess communication and parallel execution at run
  // time. There are three different groups of operators that can
  // replace the Exchange during code generation, namely,
  //
  // 1) a PartitionAccess, called a PA
  // 2) a SplitTop together with a PartitionAccess, called a PAPA
  // 3) a SplitTop, SendTop, SendBottom, SplitBottom group, which
  //    is only used when the Exchange executes within an ESP.
  //
  // The Exchange node is an enforcer for both plan execution location
  // (master, ESP, DP2) and partitioning.
  // ---------------------------------------------------------------------

  // ---------------------------------------------------------------------
  // Get the partitioning function for the top and bottom. The methods are
  // used by the (preCode) generator to avoid accessing physical props
  // directly.
  // ---------------------------------------------------------------------
  inline const PartitioningFunction * getTopPartitioningFunction() const
                                                  { return topPartFunc_; }
  inline const PartitioningFunction * getBottomPartitioningFunction() const
                                               { return bottomPartFunc_; }

  inline const IndexDesc * getIndexDesc() const     { return indexDesc_; }

  // ---------------------------------------------------------------------
  // Location for the plan rooted in this Exchange operator.
  // NOTE: both methods return FALSE if the location is undetermined.
  // ---------------------------------------------------------------------
  inline NABoolean isDP2Exchange() const
       { return (bottomLocIsSet_ AND bottomLocation_ == EXECUTE_IN_DP2); }
  inline NABoolean isEspExchange() const
       { return (bottomLocIsSet_ AND bottomLocation_ != EXECUTE_IN_DP2); }

  // ---------------------------------------------------------------------
  // Misc. methods required by the optimizer
  // ---------------------------------------------------------------------
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = NULL);

  virtual NABoolean isLogical() const;
  virtual NABoolean isPhysical() const;

  // Cascades-related functions
  virtual CostMethod* costMethod() const;
  virtual Context* createContextForAChild(Context* myContext,
                     PlanWorkSpace* pws,
                     Lng32& childIndex);

  virtual PhysicalProperty* synthPhysicalProperty(const Context *context,
                                                  const Lng32    planNumber,
                                                  PlanWorkSpace  *pws);

  // ---------------------------------------------------------------------
  // Methods used by the generator.
  // ---------------------------------------------------------------------
  virtual RelExpr * preCodeGen(Generator * generator,
			       const ValueIdSet & externalInputs,
			       ValueIdSet &pulledNewInputs);

  virtual short codeGen(Generator * generator);

  // -----------------------------------------------------
  // generate CONTROL QUERY SHAPE fragment for this node.
  // -----------------------------------------------------
  virtual short generateShape(CollHeap * space, char * buf, NAString * shapeStr = NULL);

  // ---------------------------------------------------------------------
  // The term partition input values is used for the values that
  // define the partitioning boundaries for each data stream that
  // flow into the bottom of the Exchange. The Exchange has to
  // propagate the values down to the source of the data streams,
  // which is either another Exchange or a DP2Scan.
  // ---------------------------------------------------------------------
  inline const ValueIdList &getBottomPartitionInputValues() const
                                        { return bottomPartInputValues_; }
  inline void setBottomPartitionInputValues(const ValueIdList &v)
                                           { bottomPartInputValues_ = v; }

  // ---------------------------------------------------------------------
  // get a printable string that identifies the operator
  // ---------------------------------------------------------------------
  virtual const NAString getText() const;

  // ---------------------------------------------------------------------
  // add all the expressions that are local to this
  // node to an existing list of expressions (used by GUI tool)
  // ---------------------------------------------------------------------
  virtual void addLocalExpr(LIST(ExprNode *) &xlist,
			    LIST(NAString) &llist) const;

  ExplainTuple *addSpecificExplainInfo(ExplainTupleMaster *explainTuple,
					      ComTdb * tdb,
					      Generator *generator);


  void  computeBufferLength(const Context*,
                            const CostScalar&,
                            const CostScalar&,
                            CostScalar&,
                            CostScalar&);

  inline void setDP2TransactionIndicator( NABoolean b )
                               { DP2TransactionIndicator_ = b; }
  inline NABoolean getDP2TransactionIndicator()
                               { return DP2TransactionIndicator_; }

  inline NABoolean isOverReverseScan() { return isOverReverseScan_ == TRUE;}
  void setOverReverseScan() { isOverReverseScan_=TRUE;}

  virtual PlanPriority computeOperatorPriority
   (const Context* context,
   PlanWorkSpace *pws=NULL,
   Lng32 planNumber=0);

  // for Hash2 or Hash1 pf functions, used in costing of probes
  NABoolean areProbesHashed(const ValueIdSet);

  // Does this exchange node only change # of partitions
  // the top and bottom partitioing key is still the same
  inline NABoolean hash2RepartitioningWithSameKey()
  { return hash2RepartitioningWithSameKey_;}

  inline void setNumBMOs(unsigned short num) { numBMOs_ = num; }
  inline unsigned short getNumBMOs() { return numBMOs_; }

  // set and get the total BMO memory usage of the fragment rooted
  // at this Exchange node
  inline void setBMOsMemoryUsage(CostScalar x) { BMOsMemoryUsage_ = x; }
  inline CostScalar getBMOsMemoryUsage() { return BMOsMemoryUsage_ ; }

  virtual CostScalar getEstimatedRunTimeMemoryUsage(NABoolean perCPU);
  virtual double getEstimatedRunTimeMemoryUsage(ComTdb * tdb);

  void setExtractProducerFlag() { isExtractProducer_ = TRUE; }
  NABoolean getExtractProducerFlag() { return isExtractProducer_; }

  void setExtractConsumerFlag() { isExtractConsumer_ = TRUE; }
  NABoolean getExtractConsumerFlag() { return isExtractConsumer_; }

  // Only valid after preCodeGen.  Halloween-related.
  NABoolean doesMerge() {return (sortKeyForMyOutput_.entries() != 0); }

  // Halloween-related.
  inline void markAsHalloweenProtection() 
                                  { forcedHalloweenProtection_ = TRUE; }

  inline void markHalloweenSortIsMyChild() { halloweenSortIsMyChild_ = TRUE; }

  // Three mutators for ESP Exchanges inserted during preCodeGen.   
  // Halloween-related.
  void doSkipRedundancyCheck() { skipRedundancyCheck_ = TRUE; }
  void setUpMessageBufferLength(CostScalar len) 
    { upMessageBufferLength_ = len; }
  void setDownMessageBufferLength( CostScalar len)
    { downMessageBufferLength_ = len; }

  inline NABoolean isAnESPAccess() const
  { return isAnESPAccess_;}

  inline void makeAnESPAccess()
  { isAnESPAccess_ = TRUE;}

  ExpTupleDesc::TupleDataFormat determineInternalFormat( const ValueIdList & valIdList,
                                                         RelExpr * relExpr,
                                                         NABoolean & resizeCifRecord,
                                                         Generator * generator,
                                                         NABoolean bmo_affinity,
                                                         NABoolean & considerBufferDefrag);

;


  // dop reduction
  void prepareDopReduction(Generator*);
  void doDopReduction();

  pcgEspFragment* getEspFragPCG() { return &pcgEspFragment_; };

  // Rules for enabling SeaMonster are encapsulated in this
  // method. Rules include: CQD SEAMONSTER ON or the env var
  // SQ_SEAMONSTER=1 is set, the exchange cannot be an extract
  // producer or consumer, and others.
  bool thisExchangeCanUseSM(BindWA *) const;

private:

  // ---------------------------------------------------------------------
  // PRIVATE METHODS
  // ---------------------------------------------------------------------

  // ---------------------------------------------------------------------
  // generate code (two different cases, for an ESP or for reading
  // a partitioned table in parallel)
  // ---------------------------------------------------------------------
  short codeGenForSplitTop(Generator * generator);
  short codeGenForESP(Generator * generator);
  void storePhysPropertiesInNode(const ValueIdList & );
  short generateMergeExpr(Generator * generator);


  // ---------------------------------------------------------------------
  // A method that interprets the CONTROL QUERY SHAPE ... to decide
  // what is desired by the user. The method modifies the required
  // properties for the child or sometimes even indicates that the
  // given plan should not be considered by returning NULL.
  // ---------------------------------------------------------------------
  ReqdPhysicalProperty * processCQS(const ReqdPhysicalProperty *rppForMe,
				    ReqdPhysicalProperty *rppForChild);

  // what kind of exchange is this
  // (both may return FALSE if this has not been determined yet)
  NABoolean isAPA() const;
  NABoolean isAPAPA() const;

  // Synthesize physical properties for exchange operator's current plan
  // extracted from a spcified context when child executes in DP2.
  //
  // Helper method for Exchange::synthPhysicalProperty()
  //
  PhysicalProperty*
  synthPhysicalPropertyDP2(const Context *myContext);

  // Synthesize physical properties for exchange operator's current plan
  // extracted from a spcified context when child executes in ESP.
  //
  // Helper method for Exchange::synthPhysicalProperty()
  //
  PhysicalProperty*
  synthPhysicalPropertyESP(const Context *myContext);

  // Helper method for Exchange::synthPhysicalProperty()
  // Synthesize those physical properties for exchange operator's that are common
  // to both case (child executes in DP2 and child executes in DP2)
  //
  PhysicalProperty*
  synthPhysicalPropertyFinalize(const Context *myContext,
                                PartitioningFunction *myPartFunc,
                                SortOrderTypeEnum sortOrderType,
                                PartitioningFunction *childPartFunc,
                                const PhysicalProperty *sppOfChild,
                                const ReqdPhysicalProperty *rppForMe,
                                PartitioningFunction* dp2SortOrderPartFunc);



private:

  // ---------------------------------------------------------------------
  // PRIVATE DATA: this data is stored in the Exchange node only AFTER
  // optimization. Before that time this data should be obtained from
  // the physical properties of the Exchange and/or its child.
  //
  // We have the policy that the code generator should not access
  // physical properties and therefore we store all the necessary data
  // for the code generator in private data members (see method
  // storePhysPropertiesInNode()).
  // ---------------------------------------------------------------------

  // ---------------------------------------------------------------------
  // Top and Bottom partitioning functions and bottom location
  // (set after physical properties are synthesized)
  // ---------------------------------------------------------------------
  const PartitioningFunction *topPartFunc_;
  const PartitioningFunction *bottomPartFunc_;
  NABoolean bottomLocIsSet_;
  PlanExecutionEnum bottomLocation_;
  NABoolean isRedundant_;
  NABoolean skipRedundancyCheck_;
  NABoolean isOverReverseScan_;

  const IndexDesc* indexDesc_;
  const SearchKey* partSearchKey_;

  // ---------------------------------------------------------------------
  // The bottomPartInputValues_ describes the layout of the partition
  // input values as they are sent to the child operators.
  //
  // For example, if a range partitioning function is used, then the
  // values are the begin key and the end key values for a given range.
  // For a range partitioning function, partition input values also
  // contain a boolean indicator
  // whether the end key is included (for the last key interval) or
  // excluded (all other intervals). Note that the begin key is always
  // included in the range. For example, if a partitioning key
  // contains four columns, the layout of the begin and end key values
  // in bottomPartInputValues_ is as shown below:
  //
  //   ----------------------------------------------------------------
  //   | bkv1 | bkv2 | bkv3 | bkv4 | ekv1 | ekv2 | ekv3 | ekv4 | excl |
  //   ----------------------------------------------------------------
  //   |<--- begin key values ---->|<----- end key values ---->|
  //
  // For a hash partitioning function, the part. input values contain
  // two integer values, indicating a range of hash classes.
  // ---------------------------------------------------------------------
  ValueIdList bottomPartInputValues_;   // values identifying the partition

  // ---------------------------------------------------------------------
  // If the required physical properties for this Exchange specify a
  // sort key or an arragement and the plan for the child that executes
  // in DP2 satisfies this requirement, then the Exchange cannot use
  // a non-blocking access on the child. The Exchange performs a logical
  // merge of the sorted data streams and produces an ordered stream of
  // rows as its own output. The sort key is provided by the child.
  // ---------------------------------------------------------------------
  ValueIdList sortKeyForMyOutput_;        // sort order produced by executor

  NABoolean DP2TransactionIndicator_; // set for compound statements

  unsigned short numBMOs_; // number of BMOs in this fragment

  CostScalar BMOsMemoryUsage_; // total amount of BMO memory usage in this fragment

  // Flag that indicates this exchange node only change # of partitions
  // the top and bottom partitioing key is still the same
  NABoolean hash2RepartitioningWithSameKey_;

  // For parallel extract. These fields are set during preCodeGen for
  // use by codeGen methods. Prior to preCodeGen these fields should
  // not be used.
  ValueIdList *extractSelectList_;
  NABoolean isExtractProducer_;
  NABoolean isExtractConsumer_;

  // calculated for Esp exchanges only in the optimizer
  CostScalar upMessageBufferLength_;
  CostScalar downMessageBufferLength_;

  // This flag tells if this (ESP) exchange was introduced specifically 
  // to protect against  halloween problem.  Part of solution 10-071204-9253.
  // See comments on generator/Generator.cpp.
  NABoolean forcedHalloweenProtection_;

  // This flag used in preCodeGen, in case Exchange is eliminated b/c is 
  // is redundant.
  NABoolean halloweenSortIsMyChild_;

   NABoolean isAnESPAccess_;

  // a pre-code-gen phase fragment rooted at this exchange
  pcgEspFragment pcgEspFragment_;

}; // class Exchange

#endif /* RELENFORCER_H */

