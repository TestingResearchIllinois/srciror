#include <fstream>
#include <set>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include <vector>

#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Lex/Lexer.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Rewrite/Frontend/Rewriters.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Refactoring.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/Core/Replacement.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::driver;
using namespace clang::tooling;

static llvm::cl::OptionCategory OptimuteMutatorCategory("Optimute Mutator");
static llvm::cl::opt<std::string> CoverageInfo("coverage_info", llvm::cl::desc("Specify the lines covered by the test suite and should mutate"), llvm::cl::value_desc("string"));
static std::set<int> CovSet;

class IfCondHandler : public MatchFinder::MatchCallback {

public:
  IfCondHandler(Replacements* Replace, std::string Binder, CompilerInstance* TheCompInst) : Replace(Replace), CI(TheCompInst) {
    this->Binder = Binder;
  }

  virtual void run(const MatchFinder::MatchResult &Result) {
    if (const IfStmt *CondStmt = Result.Nodes.getNodeAs<clang::IfStmt>(Binder)) {
      int lineNumber = Result.SourceManager->getSpellingLineNumber(CondStmt->getIfLoc());
      if (CovSet.find(lineNumber) == CovSet.end()) {
        // printf("The match we found: %i is not in the set of covered lines\n", lineNumber);
        return;
      }
      // printf("Found the line %i and it is covered\n", lineNumber);
      const Expr *Condition = CondStmt->getCond();
      bool invalid;

      CharSourceRange conditionRange = CharSourceRange::getTokenRange(Condition->getLocStart(), Condition->getLocEnd());
      StringRef str = Lexer::getSourceText(conditionRange, *(Result.SourceManager), CI->getLangOpts(), &invalid);
      std::string MutatedString = "!(" + str.str() + ")";
      Replacement Rep(*(Result.SourceManager), Condition->getLocStart(), str.str().size(), MutatedString);
      Replace->insert(Rep);
    }
  }

private:
  Replacements *Replace;
  CompilerInstance *CI;
  std::string Binder;
};

class WhileCondHandler : public MatchFinder::MatchCallback {

public:
  WhileCondHandler(Replacements* Replace, std::string Binder, CompilerInstance* TheCompInst) : Replace(Replace), CI(TheCompInst) {
    this->Binder = Binder;
  }

  virtual void run(const MatchFinder::MatchResult &Result) {
    if (const WhileStmt *CondStmt = Result.Nodes.getNodeAs<clang::WhileStmt>(Binder)) {
      int lineNumber = Result.SourceManager->getSpellingLineNumber(CondStmt->getWhileLoc());
      if (CovSet.find(lineNumber) == CovSet.end()) {
        // printf("The match we found: %i is not in the set of covered lines\n", lineNumber);
        return;
      }
      // printf("Found the line %i and it is covered\n", lineNumber);
      const Expr *Condition = CondStmt->getCond();
      bool invalid;

      CharSourceRange conditionRange = CharSourceRange::getTokenRange(Condition->getLocStart(), Condition->getLocEnd());  // Getting char size of condition
      StringRef str = Lexer::getSourceText(conditionRange, *(Result.SourceManager), CI->getLangOpts(), &invalid);
      std::string MutatedString = "!(" + str.str() + ")";
      Replacement Rep(*(Result.SourceManager), Condition->getLocStart(), str.str().size(), MutatedString);
      Replace->insert(Rep);
    }
  }

private:
  Replacements *Replace;
  CompilerInstance *CI;
  std::string Binder;
};

class ConstHandler : public MatchFinder::MatchCallback {

public:
  ConstHandler(Replacements* Replace, std::string Binder) : Replace(Replace) {
    this->Binder = Binder;
  }

  virtual void run(const MatchFinder::MatchResult &Result) {

    if (const IntegerLiteral *IntLiteral = Result.Nodes.getNodeAs<clang::IntegerLiteral>(Binder)) {
      int lineNumber = Result.SourceManager->getSpellingLineNumber(IntLiteral->getLocation());
      if (CovSet.find(lineNumber) == CovSet.end()) {
        // printf("The match we found: %i is not in the set of covered lines\n", lineNumber);
        return;
      }
      // printf("Found the line %i and it is covered\n", lineNumber);

      std::string ValueStr = IntLiteral->getValue().toString(10, true);
      char* endptr;
      long long Value = std::strtoll(ValueStr.c_str(), &endptr, 10);
      int Size = ValueStr.size();
      std::vector<std::string> Values;
      if (Value == 1) {
        Values.insert(Values.end(), {"(-1)", "0", "2"});
      }
      else if (Value == -1) {
        Values.insert(Values.end(), {"1", "0", "(-2)"});
      }
      else if (Value == 0) {
        Values.insert(Values.end(), {"1", "(-1)"});
      }
      else {
        Values.insert(Values.end(), {"0", "1", "(-1)", "-(" + std::to_string(Value) + ")", std::to_string(Value + 1), std::to_string(Value - 1)});
      }
      for (std::string MutationVal : Values) {
        // printf("subsituting for the value: %s\n", MutationVal.c_str());
        Replacement Rep(*(Result.SourceManager), IntLiteral->getLocStart(), Size, MutationVal);
        Replace->insert(Rep);
      }
    }
  }

private:
  Replacements *Replace;
  std::string Binder;
};




class BinaryOpHandler : public MatchFinder::MatchCallback {

public:
  BinaryOpHandler(Replacements* Replace, std::string OpName, std::string Op, std::string OpType) : Replace(Replace) {
    this->OpName = OpName;
    this->Op = Op;
    this->OpType = OpType;
    if (OpType == "arith") {
      CurrOps = &ArithmeticOperators;
    }
    else if (OpType == "rel") {
      CurrOps = &RelationalOperators;
    }
    else if (OpType == "logical") {
      CurrOps = &LogicalOperators;
    }
    else if (OpType == "bitwise") {
      CurrOps = &BitwiseOperators;
    }
    else if (OpType == "arithAssign") {
      CurrOps = &ArithAssignOperators;
    }
    else if (OpType == "bitwiseAssign") {
      CurrOps = &BitwiseAssignOperators;
    }
  }

  virtual void run(const MatchFinder::MatchResult &Result) {

    if (const BinaryOperator *BinOp = Result.Nodes.getNodeAs<clang::BinaryOperator>(OpName)) {
      int lineNumber = Result.SourceManager->getSpellingLineNumber(BinOp->getOperatorLoc());
      if (CovSet.find(lineNumber) == CovSet.end()) {
        // printf("The match we found: %i is not in the set of covered lines\n", lineNumber);
        return;
      }
      // printf("Found the line %i and it is covered\n", lineNumber);

      int Size = Op.size();
      for (std::string MutationOp : *CurrOps) {
        if (MutationOp == Op) {
          continue;
        }
        Replacement Rep(*(Result.SourceManager), BinOp->getOperatorLoc(), Size, MutationOp);
        Replace->insert(Rep);
      }
    }
  }

private:
  Replacements *Replace;
  std::string OpName;
  std::string Op;
  std::string OpType;
  std::vector<std::string>* CurrOps;
  std::vector<std::string> ArithmeticOperators = {"+", "-", "*", "/", "%"};
  std::vector<std::string> RelationalOperators = {"<", "<=", ">", ">=", "==", "!="};
  std::vector<std::string> LogicalOperators = {"&&", "||"};
  std::vector<std::string> BitwiseOperators = {"&", "|", "^"};
  std::vector<std::string> ArithAssignOperators = {"+=", "-=", "*=", "/=", "%="};
  std::vector<std::string> BitwiseAssignOperators = {"&=", "|=", "^="};

};


bool applyReplacement(const Replacement &Replace, Rewriter &Rewrite) {
  bool Result = true;
  if (Replace.isApplicable()) {
    Result = Replace.apply(Rewrite);
  }
  else {
    Result = false;
  }

  return Result;
}


void Mutate(Replacements& repl, std::string NamePrefix, std::string NameSuffix, std::string tool, std::string ext, std::string srcDir, SourceManager& SourceMgr, CompilerInstance& TheCompInst, FileManager& FileMgr){
  int x = 0;
  for (auto &r : repl) {
    x++;
    std::string pName = r.getFilePath().str();
    std::string fName = tool + ext;
    if (pName.length() >= fName.length()) {
      if (pName.compare(pName.length() - fName.length(), fName.length(), fName) == 0) {
        Rewriter TheRewriter;
        TheRewriter.setSourceMgr(SourceMgr, TheCompInst.getLangOpts());

        // Set the main file handled by the source manager to the input file.
        const FileEntry *FileIn = FileMgr.getFile(srcDir + "/" + fName);
        SourceMgr.setMainFileID(
            SourceMgr.createFileID(FileIn, SourceLocation(), SrcMgr::C_User));
        TheCompInst.getDiagnosticClient().BeginSourceFile(
            TheCompInst.getLangOpts(), &TheCompInst.getPreprocessor());

        applyReplacement(r, TheRewriter);
        const RewriteBuffer *RewriteBuf = TheRewriter.getRewriteBufferFor(SourceMgr.getMainFileID());
        std::ofstream out_file;
        std::string outFileName = srcDir + "/" + tool + ".mut." + NamePrefix + std::to_string(x) + "." + std::to_string(r.getOffset()) + "." + NameSuffix + ext;
        if (access(outFileName.c_str(), F_OK) == -1) {
          out_file.open(outFileName);
          out_file << std::string(RewriteBuf->begin(), RewriteBuf->end());
          out_file.close();
          SourceLocation startLoc = SourceMgr.getLocForStartOfFile(SourceMgr.getMainFileID());
          SourceLocation mutloc = startLoc.getLocWithOffset(r.getOffset());
          int lineNumber = SourceMgr.getSpellingLineNumber(mutloc);
          printf("line: %i %s\n", lineNumber, (tool + ".mut." + NamePrefix + std::to_string(x) + "." + std::to_string(r.getOffset()) + "." + NameSuffix + ext).c_str());    // Outputting where mutants are
        }
        else {
          printf("ERROR IN GENERATING MUTANTS: we have a name overlap for %s\n", outFileName.c_str());
        }
      }
    }
  }
}


std::set<int> parseCoverageLines(std::string ListOfLines) {
  std::stringstream ss(ListOfLines);
  std::set<int> result;

  while (ss.good()) {
    std::string substr;
    getline(ss, substr, ',');
    result.insert(std::stoi(substr));
  }

  return result;
}


int main(int argc, const char **argv) {
  CommonOptionsParser op(argc, argv, OptimuteMutatorCategory);
  RefactoringTool AORTool(op.getCompilations(), op.getSourcePathList());
  RefactoringTool RORTool(op.getCompilations(), op.getSourcePathList());
  RefactoringTool ICRTool(op.getCompilations(), op.getSourcePathList());
  RefactoringTool LCRTool(op.getCompilations(), op.getSourcePathList());
  RefactoringTool OCNGTool(op.getCompilations(), op.getSourcePathList());


  CompilerInstance TheCompInst;
  TheCompInst.createDiagnostics();

  LangOptions &lo = TheCompInst.getLangOpts();
  lo.CPlusPlus = 1; // Works on C++ code

  // Initialize target info with the default triple for our platform.
  auto TO = std::make_shared<TargetOptions>();
  TO->Triple = llvm::sys::getDefaultTargetTriple();
  TargetInfo *TI =  TargetInfo::CreateTargetInfo(TheCompInst.getDiagnostics(), TO);
  TheCompInst.setTarget(TI);

  TheCompInst.createFileManager();
  FileManager &FileMgr = TheCompInst.getFileManager();
  TheCompInst.createSourceManager(FileMgr);
  SourceManager &SourceMgr = TheCompInst.getSourceManager();
  TheCompInst.createPreprocessor(TU_Module);
  TheCompInst.createASTContext();



  // Set up AST matcher callbacks.
  BinaryOpHandler HandlerForAddOp(&AORTool.getReplacements(), "addOp", "+", "arith");
  BinaryOpHandler HandlerForSubOp(&AORTool.getReplacements(), "subOp", "-", "arith");
  BinaryOpHandler HandlerForMulOp(&AORTool.getReplacements(), "mulOp", "*", "arith");
  BinaryOpHandler HandlerForDivOp(&AORTool.getReplacements(), "divOp", "/", "arith");
  BinaryOpHandler HandlerForModuloOp(&AORTool.getReplacements(), "moduloOp", "%", "arith");

  BinaryOpHandler HandlerForLtOp(&RORTool.getReplacements(), "ltOp", "<", "rel");
  BinaryOpHandler HandlerForLeOp(&RORTool.getReplacements(), "leOp", "<=", "rel");
  BinaryOpHandler HandlerForGtOp(&RORTool.getReplacements(), "gtOp", ">", "rel");
  BinaryOpHandler HandlerForGeOp(&RORTool.getReplacements(), "geOp", ">=", "rel");
  BinaryOpHandler HandlerForEqOp(&RORTool.getReplacements(), "eqOp", "==", "rel");
  BinaryOpHandler HandlerForNeqOp(&RORTool.getReplacements(), "neqOp", "!=", "rel");

  BinaryOpHandler HandlerForAndOp(&LCRTool.getReplacements(), "logicAndOp", "&&", "logical");
  BinaryOpHandler HandlerForOrOp(&LCRTool.getReplacements(), "logicOrOp", "||", "logical");

  BinaryOpHandler HandlerForBitAndOp(&LCRTool.getReplacements(), "bitwiseAndOp", "&", "bitwise");
  BinaryOpHandler HandlerForBitOrOp(&LCRTool.getReplacements(), "bitwiseOrOp", "|", "bitwise");
  BinaryOpHandler HandlerForBitXorOp(&LCRTool.getReplacements(), "bitwiseXorOp", "^", "bitwise");

  BinaryOpHandler HandlerForAddAssignOp(&AORTool.getReplacements(), "addAssignOp", "+=", "arithAssign");
  BinaryOpHandler HandlerForSubAssignOp(&AORTool.getReplacements(), "subAssignOp", "-=", "arithAssign");
  BinaryOpHandler HandlerForMulAssignOp(&AORTool.getReplacements(), "mulAssignOp", "*=", "arithAssign");
  BinaryOpHandler HandlerForDivAssignOp(&AORTool.getReplacements(), "divAssignOp", "/=", "arithAssign");
  BinaryOpHandler HandlerForModuloAssignOp(&AORTool.getReplacements(), "moduloAssignOp", "%=", "arithAssign");


  BinaryOpHandler HandlerForAndAssignOp(&LCRTool.getReplacements(), "andAssignOp", "&=", "bitwiseAssign");
  BinaryOpHandler HandlerForOrAssignOp(&LCRTool.getReplacements(), "orAssignOp", "|=", "bitwiseAssign");
  BinaryOpHandler HandlerForXorAssignOp(&LCRTool.getReplacements(), "xorAssignOp", "^=", "bitwiseAssign");

  ConstHandler HandlerForConst(&ICRTool.getReplacements(), "intConst");
  IfCondHandler HandlerForIf(&OCNGTool.getReplacements(), "if", &TheCompInst);
  WhileCondHandler HandlerForWhile(&OCNGTool.getReplacements(), "while", &TheCompInst);

  MatchFinder AORFinder;
  AORFinder.addMatcher(binaryOperator(hasOperatorName("+")).bind("addOp"), &HandlerForAddOp);
  AORFinder.addMatcher(binaryOperator(hasOperatorName("-")).bind("subOp"), &HandlerForSubOp);
  AORFinder.addMatcher(binaryOperator(hasOperatorName("*")).bind("mulOp"), &HandlerForMulOp);
  AORFinder.addMatcher(binaryOperator(hasOperatorName("/")).bind("divOp"), &HandlerForDivOp);
  AORFinder.addMatcher(binaryOperator(hasOperatorName("%")).bind("moduloOp"), &HandlerForModuloOp);

  AORFinder.addMatcher(binaryOperator(hasOperatorName("+=")).bind("addAssignOp"), &HandlerForAddAssignOp);
  AORFinder.addMatcher(binaryOperator(hasOperatorName("-=")).bind("subAssignOp"), &HandlerForSubAssignOp);
  AORFinder.addMatcher(binaryOperator(hasOperatorName("*=")).bind("mulAssignOp"), &HandlerForMulAssignOp);
  AORFinder.addMatcher(binaryOperator(hasOperatorName("/=")).bind("divAssignOp"), &HandlerForDivAssignOp);
  AORFinder.addMatcher(binaryOperator(hasOperatorName("%=")).bind("moduloAssignOp"), &HandlerForModuloAssignOp);


  MatchFinder RORFinder;
  RORFinder.addMatcher(binaryOperator(hasOperatorName("<")).bind("ltOp"), &HandlerForLtOp);
  RORFinder.addMatcher(binaryOperator(hasOperatorName("<=")).bind("leOp"), &HandlerForLeOp);
  RORFinder.addMatcher(binaryOperator(hasOperatorName(">")).bind("gtOp"), &HandlerForGtOp);
  RORFinder.addMatcher(binaryOperator(hasOperatorName(">=")).bind("geOp"), &HandlerForGeOp);
  RORFinder.addMatcher(binaryOperator(hasOperatorName("==")).bind("eqOp"), &HandlerForEqOp);
  RORFinder.addMatcher(binaryOperator(hasOperatorName("!=")).bind("neqOp"), &HandlerForNeqOp);

  MatchFinder LCRFinder;
  LCRFinder.addMatcher(binaryOperator(hasOperatorName("&&")).bind("logicAndOp"), &HandlerForAndOp);
  LCRFinder.addMatcher(binaryOperator(hasOperatorName("||")).bind("logicOrOp"), &HandlerForOrOp);

  LCRFinder.addMatcher(binaryOperator(hasOperatorName("&")).bind("bitwiseAndOp"), &HandlerForBitAndOp);
  LCRFinder.addMatcher(binaryOperator(hasOperatorName("|")).bind("bitwiseOrOp"), &HandlerForBitOrOp);
  LCRFinder.addMatcher(binaryOperator(hasOperatorName("^")).bind("bitwiseXorOp"), &HandlerForBitXorOp);


  LCRFinder.addMatcher(binaryOperator(hasOperatorName("&=")).bind("andAssignOp"), &HandlerForAndAssignOp);
  LCRFinder.addMatcher(binaryOperator(hasOperatorName("|=")).bind("orAssignOp"), &HandlerForOrAssignOp);
  LCRFinder.addMatcher(binaryOperator(hasOperatorName("^=")).bind("xorAssignOp"), &HandlerForXorAssignOp);

  MatchFinder ICRFinder;
  ICRFinder.addMatcher(integerLiteral().bind("intConst"), &HandlerForConst);

  MatchFinder OCNGFinder;
  OCNGFinder.addMatcher(ifStmt().bind("if"), &HandlerForIf);
  OCNGFinder.addMatcher(whileStmt().bind("while"), &HandlerForWhile);

  std::string FileName = argv[1];   // Assumes only one source file on command line to mutate
  CovSet = parseCoverageLines(CoverageInfo);
  std::string ToolDotExt = FileName.substr(FileName.find_last_of("/\\") + 1);
  std::string::size_type const p(ToolDotExt.find_last_of('.'));
  std::string CurrTool = ToolDotExt.substr(0, p);
  std::string Ext = ToolDotExt.substr(p, ToolDotExt.length());
  std::string SrcDir = FileName.substr(0, FileName.find_last_of("/\\"));

  int failed = 0;
  if (int Result = AORTool.run(newFrontendActionFactory(&AORFinder).get())) {
    failed = 1;
  }
  else {
    Mutate(AORTool.getReplacements(), "binaryop_for_", "60", CurrTool, Ext, SrcDir, SourceMgr, TheCompInst, FileMgr);
  }
  //-----
  if (int Result = RORTool.run(newFrontendActionFactory(&RORFinder).get())) {
    failed = 1;
  }
  else {
    Mutate(RORTool.getReplacements(), "binaryop_for_", "61", CurrTool, Ext, SrcDir, SourceMgr, TheCompInst, FileMgr);
  }
  //-----
  if (int Result = ICRTool.run(newFrontendActionFactory(&ICRFinder).get())) {
    failed = 1;
  }
  else {
    Mutate(ICRTool.getReplacements(), "const_for_", "const", CurrTool, Ext, SrcDir, SourceMgr, TheCompInst, FileMgr);
  }
  //-----
  if (int Result = LCRTool.run(newFrontendActionFactory(&LCRFinder).get())) {
    failed = 1;
  }
  else {
    Mutate(LCRTool.getReplacements(), "binaryop_for_", "62", CurrTool, Ext, SrcDir, SourceMgr, TheCompInst, FileMgr);
  }
  //-----
  if (int Result = OCNGTool.run(newFrontendActionFactory(&OCNGFinder).get())) {
    failed = 1;
  }
  else {
    Mutate(OCNGTool.getReplacements(), "binaryop_for_", "ocng", CurrTool, Ext, SrcDir, SourceMgr, TheCompInst, FileMgr);
  }

  return failed;
}
