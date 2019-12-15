// swad_test.h: self-assessment tests

#ifndef _SWAD_TST
#define _SWAD_TST
/*
    SWAD (Shared Workspace At a Distance in Spanish),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2019 Antonio Ca�as Vargas

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/*****************************************************************************/
/********************************* Headers ***********************************/
/*****************************************************************************/

#include "swad_game.h"

/*****************************************************************************/
/***************************** Public constants ******************************/
/*****************************************************************************/

#define Tst_SCORE_MAX				  10	// Maximum score of a test (10 in Spain). Must be unsigned! // TODO: Make this configurable by teachers

#define Tst_MAX_QUESTIONS_PER_TEST		 100	// Absolute maximum number of questions in a test
#define Tst_MAX_TAGS_PER_QUESTION		   5

#define Tst_MAX_CHARS_TAG			(128 - 1)	// 127
#define Tst_MAX_BYTES_TAG			((Tst_MAX_CHARS_TAG + 1) * Str_MAX_BYTES_PER_CHAR - 1)	// 2047

#define Tst_MAX_OPTIONS_PER_QUESTION		  10
#define Tst_MAX_BYTES_INDEXES_ONE_QST		(Tst_MAX_OPTIONS_PER_QUESTION * (10 + 1))
#define Tst_MAX_BYTES_ANSWERS_ONE_QST		(Tst_MAX_OPTIONS_PER_QUESTION * (10 + 1))

#define Tst_MAX_CHARS_ANSWER_OR_FEEDBACK	(1024 - 1)	// 1023
#define Tst_MAX_BYTES_ANSWER_OR_FEEDBACK	((Tst_MAX_CHARS_ANSWER_OR_FEEDBACK + 1) * Str_MAX_BYTES_PER_CHAR - 1)	// 16383

#define Tst_CONFIG_DEFAULT_MIN_QUESTIONS	   1
#define Tst_CONFIG_DEFAULT_DEF_QUESTIONS	  20	// Number of questions to be generated by default in a self-assessment test
#define Tst_CONFIG_DEFAULT_MAX_QUESTIONS	  30	// Maximum number of questions to be generated in a self-assessment test

#define Tst_MAX_BYTES_FEEDBACK_TYPE		  32

#define Tst_MAX_BYTES_ANSWER_TYPE		  32

/*****************************************************************************/
/******************************* Public types ********************************/
/*****************************************************************************/

#define Tst_NUM_OPTIONS_PLUGGABLE	3
typedef enum
  {
   Tst_PLUGGABLE_UNKNOWN = 0,
   Tst_PLUGGABLE_NO      = 1,
   Tst_PLUGGABLE_YES     = 2,
  } Tst_Pluggable_t;

typedef enum
  {
   Tst_SHOW_TEST_TO_ANSWER,		// Showing a test to a student
   Tst_SHOW_TEST_RESULT,		// Showing the assessment of a test
   Tst_EDIT_TEST,			// Editing test questions
   Tst_SELECT_QUESTIONS_FOR_GAME,	// Selecting test questions for a game
  } Tst_ActionToDoWithQuestions_t;

#define Tst_NUM_TYPES_FEEDBACK		5
typedef enum
  {
   Tst_FEEDBACK_NOTHING        = 0,
   Tst_FEEDBACK_TOTAL_RESULT   = 1,
   Tst_FEEDBACK_EACH_RESULT    = 2,
   Tst_FEEDBACK_EACH_GOOD_BAD  = 3,
   Tst_FEEDBACK_FULL_FEEDBACK  = 4,
  } Tst_Feedback_t;
#define Tst_FEEDBACK_DEFAULT Tst_FEEDBACK_FULL_FEEDBACK

struct Tst_Config
  {
   Tst_Pluggable_t Pluggable;
   unsigned Min;	// Minimum number of questions
   unsigned Def;	// Default number of questions
   unsigned Max;	// Maximum number of questions
   unsigned long MinTimeNxtTstPerQst;
   Tst_Feedback_t Feedback;
  };

#define Tst_NUM_ANS_TYPES	6
#define Tst_MAX_BYTES_LIST_ANSWER_TYPES	(Tst_NUM_ANS_TYPES * (Cns_MAX_DECIMAL_DIGITS_UINT + 1))
typedef enum
  {
   Tst_ANS_INT             = 0,
   Tst_ANS_FLOAT           = 1,
   Tst_ANS_TRUE_FALSE      = 2,
   Tst_ANS_UNIQUE_CHOICE   = 3,
   Tst_ANS_MULTIPLE_CHOICE = 4,
   Tst_ANS_TEXT            = 5,
   Tst_ANS_ALL             = 6,	// All/any type of answer
  } Tst_AnswerType_t;

#define Tst_NUM_TYPES_ORDER_QST	5
typedef enum
  {
   Tst_ORDER_STEM                    = 0,
   Tst_ORDER_NUM_HITS                = 1,
   Tst_ORDER_AVERAGE_SCORE           = 2,
   Tst_ORDER_NUM_HITS_NOT_BLANK      = 3,
   Tst_ORDER_AVERAGE_SCORE_NOT_BLANK = 4,
  } Tst_QuestionsOrder_t;

struct Tst_Stats
  {
   unsigned NumCoursesWithQuestions;
   unsigned NumCoursesWithPluggableQuestions;
   unsigned NumQsts;
   double AvgQstsPerCourse;
   unsigned long NumHits;
   double AvgHitsPerCourse;
   double AvgHitsPerQuestion;
   double TotalScore;
   double AvgScorePerQuestion;
  };

/*****************************************************************************/
/***************************** Public prototypes *****************************/
/*****************************************************************************/

void Tst_ShowFormAskTst (void);
void Tst_ShowNewTest (void);
void Tst_AssessTest (void);

void Tst_ComputeAndShowGrade (unsigned NumQsts,double Score,double MaxGrade);
double Tst_ComputeGrade (unsigned NumQsts,double Score,double MaxGrade);
void Tst_ShowGrade (double Grade,double MaxGrade);

void Tst_ShowTagList (unsigned NumTags,MYSQL_RES *mysql_res);

void Tst_WriteQstStem (const char *Stem,const char *ClassStem);
void Tst_WriteQstFeedback (const char *Feedback,const char *ClassFeedback);

void Tst_ShowFormAskEditTsts (void);
void Tst_ShowFormAskSelectTstsForGame (void);
void Tst_ListQuestionsToEdit (void);
void Tst_ListQuestionsToSelect (void);
bool Tst_GetOneQuestionByCod (long QstCod,MYSQL_RES **mysql_res);
void Tst_WriteParamEditQst (void);
unsigned Tst_GetNumAnswersQst (long QstCod);
unsigned Tst_GetAnswersQst (long QstCod,MYSQL_RES **mysql_res,bool Shuffle);
void Tst_GetCorrectAnswersFromDB (long QstCod);
void Tst_WriteAnswersEdit (long QstCod);
bool Tst_CheckIfQuestionIsValidForGame (long QstCod);
void Tst_WriteAnsTF (char AnsTF);
void Tst_GetChoiceAns (MYSQL_RES *mysql_res);
void Tst_GetIndexesFromStr (const char StrIndexesOneQst[Tst_MAX_BYTES_INDEXES_ONE_QST + 1],	// 0 1 2 3, 3 0 2 1, etc.
			    unsigned Indexes[Tst_MAX_OPTIONS_PER_QUESTION]);
void Tst_GetAnswersFromStr (const char StrAnswersOneQst[Tst_MAX_BYTES_ANSWERS_ONE_QST + 1],
			    bool AnswersUsr[Tst_MAX_OPTIONS_PER_QUESTION]);
void Tst_ComputeScoreQst (unsigned Indexes[Tst_MAX_OPTIONS_PER_QUESTION],
                          bool AnswersUsr[Tst_MAX_OPTIONS_PER_QUESTION],
			  double *ScoreThisQst,bool *AnswerIsNotBlank);
void Tst_WriteChoiceAnsViewMatch (long MchCod,unsigned QstInd,long QstCod,
				  unsigned NumCols,const char *Class,bool ShowResult);
void Tst_CheckIfNumberOfAnswersIsOne (void);

unsigned long Tst_GetTagsQst (long QstCod,MYSQL_RES **mysql_res);
void Tst_GetAndWriteTagsQst (long QstCod);

void Tst_ShowFormConfig (void);
void Tst_EnableTag (void);
void Tst_DisableTag (void);
void Tst_RenameTag (void);

void Tst_GetConfigTstFromDB (void);

void Tst_GetConfigFromRow (MYSQL_ROW row);
bool Tst_CheckIfCourseHaveTestsAndPluggableIsUnknown (void);
void Tst_ReceiveConfigTst (void);
void Tst_ShowFormEditOneQst (void);

void Tst_QstConstructor (void);
void Tst_QstDestructor (void);

int Tst_AllocateTextChoiceAnswer (unsigned NumOpt);

Tst_AnswerType_t Tst_ConvertFromStrAnsTypDBToAnsTyp (const char *StrAnsTypeBD);
void Tst_ReceiveQst (void);
bool Tst_CheckIfQstFormatIsCorrectAndCountNumOptions (void);

bool Tst_CheckIfQuestionExistsInDB (void);

long Tst_GetIntAnsFromStr (char *Str);
void Tst_RequestRemoveQst (void);
void Tst_RemoveQst (void);
void Tst_ChangeShuffleQst (void);

void Tst_PutParamQstCod (void);

void Tst_InsertOrUpdateQstTagsAnsIntoDB (void);

void Tst_FreeTagsList (void);

void Tst_GetTestStats (Tst_AnswerType_t AnsType,struct Tst_Stats *Stats);

void Tst_SelUsrsToViewUsrsTstResults (void);
void Tst_SelDatesToSeeMyTstResults (void);
void Tst_ShowMyTstResults (void);
void Tst_GetUsrsAndShowTstResults (void);
void Tst_ShowOneTstResult (void);
void Tst_ShowTestResult (struct UsrData *UsrDat,
			 unsigned NumQsts,time_t TstTimeUTC);
void Tst_RemoveTestResultsMadeByUsrInAllCrss (long UsrCod);
void Tst_RemoveTestResultsMadeByUsrInCrs (long UsrCod,long CrsCod);
void Tst_RemoveCrsTestResults (long CrsCod);

void Tst_RemoveCrsTests (long CrsCod);

#endif
