// swad_match_result.c: matches results in games using remote control

/*
    SWAD (Shared Workspace At a Distance),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2021 Antonio Ca�as Vargas

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

#define _GNU_SOURCE 		// For asprintf
#include <linux/limits.h>	// For PATH_MAX
#include <stddef.h>		// For NULL
#include <stdio.h>		// For asprintf
#include <string.h>		// For string functions

#include "swad_action.h"
#include "swad_database.h"
#include "swad_date.h"
#include "swad_form.h"
#include "swad_global.h"
#include "swad_HTML.h"
#include "swad_ID.h"
#include "swad_match.h"
#include "swad_match_result.h"
#include "swad_photo.h"
#include "swad_test_visibility.h"
#include "swad_user.h"

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/***************************** Private constants *****************************/
/*****************************************************************************/

/*****************************************************************************/
/******************************* Private types *******************************/
/*****************************************************************************/

struct MchRes_ICanView
  {
   bool Result;
   bool Score;
  };

/*****************************************************************************/
/***************************** Private constants *****************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Private variables *****************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Private prototypes ****************************/
/*****************************************************************************/

static void MchRes_PutFormToSelUsrsToViewMchResults (void *Games);

static void MchRes_ListMyMchResultsInCrs (struct Gam_Games *Games);
static void MchRes_ListMyMchResultsInGam (struct Gam_Games *Games,long GamCod);
static void MchRes_ListMyMchResultsInMch (struct Gam_Games *Games,long MchCod);
static void MchRes_ShowAllMchResultsInSelectedGames (void *Games);
static void MchRes_ListAllMchResultsInSelectedGames (struct Gam_Games *Games);
static void MchRes_ListAllMchResultsInGam (struct Gam_Games *Games,long GamCod);
static void MchRes_ListAllMchResultsInMch (struct Gam_Games *Games,long MchCod);

static void MchRes_ShowResultsBegin (struct Gam_Games *Games,
                                     const char *Title,bool ListGamesToSelect);
static void MchRes_ShowResultsEnd (void);

static void MchRes_ListGamesToSelect (struct Gam_Games *Games);
static void MchRes_ShowHeaderMchResults (Usr_MeOrOther_t MeOrOther);

static void MchRes_BuildGamesSelectedCommas (struct Gam_Games *Games,
                                             char **GamesSelectedCommas);
static void MchRes_ShowMchResults (struct Gam_Games *Games,
                                   Usr_MeOrOther_t MeOrOther,
				   long MchCod,	// <= 0 ==> any
				   long GamCod,	// <= 0 ==> any
				   const char *GamesSelectedCommas);
static void MchRes_ShowMchResultsSummaryRow (unsigned NumResults,
                                             struct MchPrn_NumQuestions *NumTotalQsts,
                                             double TotalScore,
					     double TotalGrade);

static void MchRes_CheckIfICanSeeMatchResult (const struct Gam_Game *Game,
                                              const struct Mch_Match *Match,
                                              long UsrCod,
                                              struct MchRes_ICanView *ICanView);

/*****************************************************************************/
/*************************** Show my matches results *************************/
/*****************************************************************************/

void MchRes_ShowMyMchResultsInCrs (void)
  {
   extern const char *Txt_Results;
   struct Gam_Games Games;

   /***** Reset games context *****/
   Gam_ResetGames (&Games);

   /***** Get list of games *****/
   Gam_GetListGames (&Games,Gam_ORDER_BY_TITLE);
   Gam_GetListSelectedGamCods (&Games);

   /***** List my matches results in the current course *****/
   MchRes_ShowResultsBegin (&Games,Txt_Results,true);	// List games to select
   MchRes_ListMyMchResultsInCrs (&Games);
   MchRes_ShowResultsEnd ();

   /***** Free list of games *****/
   free (Games.GamCodsSelected);
   Gam_FreeListGames (&Games);
  }

static void MchRes_ListMyMchResultsInCrs (struct Gam_Games *Games)
  {
   char *GamesSelectedCommas = NULL;	// Initialized to avoid warning

   /***** Table header *****/
   MchRes_ShowHeaderMchResults (Usr_ME);

   /***** List my matches results in the current course *****/
   TstCfg_GetConfigFromDB ();	// Get feedback type
   MchRes_BuildGamesSelectedCommas (Games,&GamesSelectedCommas);
   MchRes_ShowMchResults (Games,Usr_ME,-1L,-1L,GamesSelectedCommas);
   free (GamesSelectedCommas);
  }

/*****************************************************************************/
/***************** Show my matches results in a given game *******************/
/*****************************************************************************/

void MchRes_ShowMyMchResultsInGam (void)
  {
   extern const char *Txt_Results_of_game_X;
   struct Gam_Games Games;
   struct Gam_Game Game;

   /***** Reset games context *****/
   Gam_ResetGames (&Games);

   /***** Reset game *****/
   Gam_ResetGame (&Game);

   /***** Get parameters *****/
   if ((Game.GamCod = Gam_GetParams (&Games)) <= 0)
      Lay_ShowErrorAndExit ("Code of game is missing.");
   Gam_GetDataOfGameByCod (&Game);

   /***** Game begin *****/
   Gam_ShowOnlyOneGameBegin (&Games,&Game,
                             false,	// Do not list game questions
	                     false);	// Do not put form to start new match

   /***** List my matches results in game *****/
   MchRes_ShowResultsBegin (&Games,
                            Str_BuildStringStr (Txt_Results_of_game_X,Game.Title),
			    false);	// Do not list games to select
   Str_FreeString ();
   MchRes_ListMyMchResultsInGam (&Games,Game.GamCod);
   MchRes_ShowResultsEnd ();

   /***** Game end *****/
   Gam_ShowOnlyOneGameEnd ();
  }

static void MchRes_ListMyMchResultsInGam (struct Gam_Games *Games,long GamCod)
  {
   /***** Table header *****/
   MchRes_ShowHeaderMchResults (Usr_ME);

   /***** List my matches results in game *****/
   TstCfg_GetConfigFromDB ();	// Get feedback type
   MchRes_ShowMchResults (Games,Usr_ME,-1L,GamCod,NULL);
  }

/*****************************************************************************/
/***************** Show my matches results in a given match ******************/
/*****************************************************************************/

void MchRes_ShowMyMchResultsInMch (void)
  {
   extern const char *Txt_Results_of_match_X;
   struct Gam_Games Games;
   struct Gam_Game Game;
   struct Mch_Match Match;

   /***** Reset games context *****/
   Gam_ResetGames (&Games);

   /***** Reset game and match *****/
   Gam_ResetGame (&Game);
   Mch_ResetMatch (&Match);

   /***** Get parameters *****/
   if ((Game.GamCod = Gam_GetParams (&Games)) <= 0)
      Lay_ShowErrorAndExit ("Code of game is missing.");
   if ((Match.MchCod = Mch_GetParamMchCod ()) <= 0)
      Lay_ShowErrorAndExit ("Code of match is missing.");
   Gam_GetDataOfGameByCod (&Game);
   Mch_GetDataOfMatchByCod (&Match);

   /***** Game begin *****/
   Gam_ShowOnlyOneGameBegin (&Games,&Game,
                             false,	// Do not list game questions
	                     false);	// Do not put form to start new match

   /***** List my matches results in match *****/
   MchRes_ShowResultsBegin (&Games,Str_BuildStringStr (Txt_Results_of_match_X,Match.Title),
			    false);	// Do not list games to select
   Str_FreeString ();
   MchRes_ListMyMchResultsInMch (&Games,Match.MchCod);
   MchRes_ShowResultsEnd ();

   /***** Game end *****/
   Gam_ShowOnlyOneGameEnd ();
  }

static void MchRes_ListMyMchResultsInMch (struct Gam_Games *Games,long MchCod)
  {
   /***** Table header *****/
   MchRes_ShowHeaderMchResults (Usr_ME);

   /***** List my matches results in game *****/
   TstCfg_GetConfigFromDB ();	// Get feedback type
   MchRes_ShowMchResults (Games,Usr_ME,MchCod,-1L,NULL);
  }

/*****************************************************************************/
/****************** Get users and show their matches results *****************/
/*****************************************************************************/

void MchRes_ShowAllMchResultsInCrs (void)
  {
   struct Gam_Games Games;

   /***** Reset games context *****/
   Gam_ResetGames (&Games);

   /***** Get users and show their matches results *****/
   Usr_GetSelectedUsrsAndGoToAct (&Gbl.Usrs.Selected,
				  MchRes_ShowAllMchResultsInSelectedGames,&Games,
                                  MchRes_PutFormToSelUsrsToViewMchResults,&Games);
  }

/*****************************************************************************/
/****************** Show matches results for several users *******************/
/*****************************************************************************/

static void MchRes_ShowAllMchResultsInSelectedGames (void *Games)
  {
   extern const char *Txt_Results;

   if (!Games)
      return;

   /***** Get list of games *****/
   Gam_GetListGames ((struct Gam_Games *) Games,Gam_ORDER_BY_TITLE);
   Gam_GetListSelectedGamCods ((struct Gam_Games *) Games);

   /***** List the matches results of the selected users *****/
   MchRes_ShowResultsBegin ((struct Gam_Games *) Games,
                            Txt_Results,
                            true);	// List games to select
   MchRes_ListAllMchResultsInSelectedGames ((struct Gam_Games *) Games);
   MchRes_ShowResultsEnd ();

   /***** Free list of games *****/
   free (((struct Gam_Games *) Games)->GamCodsSelected);
   Gam_FreeListGames ((struct Gam_Games *) Games);
  }

static void MchRes_ListAllMchResultsInSelectedGames (struct Gam_Games *Games)
  {
   char *GamesSelectedCommas = NULL;	// Initialized to avoid warning
   const char *Ptr;

   /***** Table head *****/
   MchRes_ShowHeaderMchResults (Usr_OTHER);

   /***** List the matches results of the selected users *****/
   MchRes_BuildGamesSelectedCommas (Games,&GamesSelectedCommas);
   Ptr = Gbl.Usrs.Selected.List[Rol_UNK];
   while (*Ptr)
     {
      Par_GetNextStrUntilSeparParamMult (&Ptr,Gbl.Usrs.Other.UsrDat.EnUsrCod,
					 Cry_BYTES_ENCRYPTED_STR_SHA256_BASE64);
      Usr_GetUsrCodFromEncryptedUsrCod (&Gbl.Usrs.Other.UsrDat);
      if (Usr_ChkUsrCodAndGetAllUsrDataFromUsrCod (&Gbl.Usrs.Other.UsrDat,Usr_DONT_GET_PREFS))
	 if (Usr_CheckIfICanViewTstExaMchResult (&Gbl.Usrs.Other.UsrDat))
	   {
	    /***** Show matches results *****/
	    Gbl.Usrs.Other.UsrDat.Accepted = Usr_CheckIfUsrHasAcceptedInCurrentCrs (&Gbl.Usrs.Other.UsrDat);
	    MchRes_ShowMchResults (Games,Usr_OTHER,-1L,-1L,GamesSelectedCommas);
	   }
     }
   free (GamesSelectedCommas);
  }

/*****************************************************************************/
/**************** Select users to show their matches results *****************/
/*****************************************************************************/

void MchRes_SelUsrsToViewMchResults (void)
  {
   struct Gam_Games Games;

   /***** Reset games context *****/
   Gam_ResetGames (&Games);

   /***** Put form to select users *****/
   MchRes_PutFormToSelUsrsToViewMchResults (&Games);
  }

static void MchRes_PutFormToSelUsrsToViewMchResults (void *Games)
  {
   extern const char *Hlp_ASSESSMENT_Games_results;
   extern const char *Txt_Results;
   extern const char *Txt_View_results;

   if (Games)	// Not used
      Usr_PutFormToSelectUsrsToGoToAct (&Gbl.Usrs.Selected,
					ActSeeUsrMchResCrs,
					NULL,NULL,
					Txt_Results,
					Hlp_ASSESSMENT_Games_results,
					Txt_View_results,
					false);	// Do not put form with date range
  }

/*****************************************************************************/
/*** Show matches results of a game for the users who answered in that game **/
/*****************************************************************************/

void MchRes_ShowAllMchResultsInGam (void)
  {
   extern const char *Txt_Results_of_game_X;
   struct Gam_Games Games;
   struct Gam_Game Game;

   /***** Reset games context *****/
   Gam_ResetGames (&Games);

   /***** Reset game *****/
   Gam_ResetGame (&Game);

   /***** Get parameters *****/
   if ((Game.GamCod = Gam_GetParams (&Games)) <= 0)
      Lay_ShowErrorAndExit ("Code of game is missing.");
   Gam_GetDataOfGameByCod (&Game);

   /***** Game begin *****/
   Gam_ShowOnlyOneGameBegin (&Games,&Game,
                             false,	// Do not list game questions
	                     false);	// Do not put form to start new match

   /***** List matches results in game *****/
   MchRes_ShowResultsBegin (&Games,
                            Str_BuildStringStr (Txt_Results_of_game_X,Game.Title),
			    false);	// Do not list games to select
   Str_FreeString ();
   MchRes_ListAllMchResultsInGam (&Games,Game.GamCod);
   MchRes_ShowResultsEnd ();

   /***** Game end *****/
   Gam_ShowOnlyOneGameEnd ();
  }

static void MchRes_ListAllMchResultsInGam (struct Gam_Games *Games,long GamCod)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned long NumUsrs;
   unsigned long NumUsr;

   /***** Table head *****/
   MchRes_ShowHeaderMchResults (Usr_OTHER);

   /***** Get all users who have answered any match question in this game *****/
   NumUsrs = DB_QuerySELECT (&mysql_res,"can not get users in game",
			     "SELECT users.UsrCod FROM"
			     " (SELECT DISTINCT mch_results.UsrCod AS UsrCod"	// row[0]
			     " FROM mch_results,mch_matches,gam_games"
			     " WHERE mch_matches.GamCod=%ld"
			     " AND mch_matches.MchCod=mch_results.MchCod"
			     " AND mch_matches.GamCod=gam_games.GamCod"
			     " AND gam_games.CrsCod=%ld)"			// Extra check
			     " AS users,usr_data"
			     " WHERE users.UsrCod=usr_data.UsrCod"
			     " ORDER BY usr_data.Surname1,"
			               "usr_data.Surname2,"
			               "usr_data.FirstName",
			     GamCod,
			     Gbl.Hierarchy.Crs.CrsCod);
   if (NumUsrs)
     {
      /***** List matches results for each user *****/
      for (NumUsr = 0;
	   NumUsr < NumUsrs;
	   NumUsr++)
	{
	 row = mysql_fetch_row (mysql_res);

	 /* Get match code (row[0]) */
	 if ((Gbl.Usrs.Other.UsrDat.UsrCod = Str_ConvertStrCodToLongCod (row[0])) > 0)
	    if (Usr_ChkUsrCodAndGetAllUsrDataFromUsrCod (&Gbl.Usrs.Other.UsrDat,Usr_DONT_GET_PREFS))
	       if (Usr_CheckIfICanViewTstExaMchResult (&Gbl.Usrs.Other.UsrDat))
		 {
		  /***** Show matches results *****/
		  Gbl.Usrs.Other.UsrDat.Accepted = Usr_CheckIfUsrHasAcceptedInCurrentCrs (&Gbl.Usrs.Other.UsrDat);
		  MchRes_ShowMchResults (Games,Usr_OTHER,-1L,GamCod,NULL);
		 }
	}
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/** Show matches results of a match for the users who answered in that match */
/*****************************************************************************/

void MchRes_ShowAllMchResultsInMch (void)
  {
   extern const char *Txt_Results_of_match_X;
   struct Gam_Games Games;
   struct Gam_Game Game;
   struct Mch_Match Match;

   /***** Reset games context *****/
   Gam_ResetGames (&Games);

   /***** Reset game and match *****/
   Gam_ResetGame (&Game);
   Mch_ResetMatch (&Match);

   /***** Get parameters *****/
   if ((Game.GamCod = Gam_GetParams (&Games)) <= 0)
      Lay_ShowErrorAndExit ("Code of game is missing.");
   if ((Match.MchCod = Mch_GetParamMchCod ()) <= 0)
      Lay_ShowErrorAndExit ("Code of match is missing.");
   Gam_GetDataOfGameByCod (&Game);
   Mch_GetDataOfMatchByCod (&Match);

   /***** Game begin *****/
   Gam_ShowOnlyOneGameBegin (&Games,&Game,
                             false,	// Do not list game questions
	                     false);	// Do not put form to start new match

   /***** List matches results in match *****/
   MchRes_ShowResultsBegin (&Games,
                            Str_BuildStringStr (Txt_Results_of_match_X,Match.Title),
			    false);	// Do not list games to select
   Str_FreeString ();
   MchRes_ListAllMchResultsInMch (&Games,Match.MchCod);
   MchRes_ShowResultsEnd ();

   /***** Game end *****/
   Gam_ShowOnlyOneGameEnd ();
  }

static void MchRes_ListAllMchResultsInMch (struct Gam_Games *Games,long MchCod)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned long NumUsrs;
   unsigned long NumUsr;

   /***** Table head *****/
   MchRes_ShowHeaderMchResults (Usr_OTHER);

   /***** Get all users who have answered any match question in this game *****/
   NumUsrs = DB_QuerySELECT (&mysql_res,"can not get users in match",
			     "SELECT users.UsrCod FROM"
			     " (SELECT mch_results.UsrCod AS UsrCod"	// row[0]
			     " FROM mch_results,mch_matches,gam_games"
			     " WHERE mch_results.MchCod=%ld"
			     " AND mch_results.MchCod=mch_matches.MchCod"
			     " AND mch_matches.GamCod=gam_games.GamCod"
			     " AND gam_games.CrsCod=%ld)"		// Extra check
			     " AS users,usr_data"
			     " WHERE users.UsrCod=usr_data.UsrCod"
			     " ORDER BY usr_data.Surname1,"
			               "usr_data.Surname2,"
			               "usr_data.FirstName",
			     MchCod,
			     Gbl.Hierarchy.Crs.CrsCod);
   if (NumUsrs)
     {
      /***** List matches results for each user *****/
      for (NumUsr = 0;
	   NumUsr < NumUsrs;
	   NumUsr++)
	{
	 row = mysql_fetch_row (mysql_res);

	 /* Get match code (row[0]) */
	 if ((Gbl.Usrs.Other.UsrDat.UsrCod = Str_ConvertStrCodToLongCod (row[0])) > 0)
	    if (Usr_ChkUsrCodAndGetAllUsrDataFromUsrCod (&Gbl.Usrs.Other.UsrDat,Usr_DONT_GET_PREFS))
	       if (Usr_CheckIfICanViewTstExaMchResult (&Gbl.Usrs.Other.UsrDat))
		 {
		  /***** Show matches results *****/
		  Gbl.Usrs.Other.UsrDat.Accepted = Usr_CheckIfUsrHasAcceptedInCurrentCrs (&Gbl.Usrs.Other.UsrDat);
		  MchRes_ShowMchResults (Games,Usr_OTHER,MchCod,-1L,NULL);
		 }
	}
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/************************ Show results (begin / end) *************************/
/*****************************************************************************/

static void MchRes_ShowResultsBegin (struct Gam_Games *Games,
                                     const char *Title,bool ListGamesToSelect)
  {
   extern const char *Hlp_ASSESSMENT_Games_results;

   /***** Begin box *****/
   HTM_SECTION_Begin (MchRes_RESULTS_BOX_ID);
   Box_BoxBegin (NULL,Title,
                 NULL,NULL,
		 Hlp_ASSESSMENT_Games_results,Box_NOT_CLOSABLE);

   /***** List games to select *****/
   if (ListGamesToSelect)
      MchRes_ListGamesToSelect (Games);

   /***** Begin match results table *****/
   HTM_SECTION_Begin (MchRes_RESULTS_TABLE_ID);
   HTM_TABLE_BeginWidePadding (5);
  }

static void MchRes_ShowResultsEnd (void)
  {
   /***** End match results table *****/
   HTM_TABLE_End ();
   HTM_SECTION_End ();

   /***** End box *****/
   Box_BoxEnd ();
   HTM_SECTION_End ();
  }

/*****************************************************************************/
/********** Write list of those attendance events that have students *********/
/*****************************************************************************/

static void MchRes_ListGamesToSelect (struct Gam_Games *Games)
  {
   extern const char *The_ClassFormLinkInBoxBold[The_NUM_THEMES];
   extern const char *Txt_Games;
   extern const char *Txt_Game;
   extern const char *Txt_Update_results;
   unsigned UniqueId;
   unsigned NumGame;
   struct Gam_Game Game;

   /***** Reset game *****/
   Gam_ResetGame (&Game);

   /***** Begin box *****/
   Box_BoxBegin (NULL,Txt_Games,
                 NULL,NULL,
                 NULL,Box_CLOSABLE);

   /***** Begin form to update the results
	  depending on the games selected *****/
   Frm_StartFormAnchor (Gbl.Action.Act,MchRes_RESULTS_TABLE_ID);
   Grp_PutParamsCodGrps ();
   Usr_PutHiddenParSelectedUsrsCods (&Gbl.Usrs.Selected);

   /***** Begin table *****/
   HTM_TABLE_BeginWidePadding (2);

   /***** Heading row *****/
   HTM_TR_Begin (NULL);

   HTM_TH (1,2,NULL,NULL);
   HTM_TH (1,1,"LM",Txt_Game);

   HTM_TR_End ();

   /***** List the events *****/
   for (NumGame = 0, UniqueId = 1, Gbl.RowEvenOdd = 0;
	NumGame < Games->Num;
	NumGame++, UniqueId++, Gbl.RowEvenOdd = 1 - Gbl.RowEvenOdd)
     {
      /* Get data of this game */
      Game.GamCod = Games->Lst[NumGame].GamCod;
      Gam_GetDataOfGameByCod (&Game);

      /* Write a row for this event */
      HTM_TR_Begin (NULL);

      HTM_TD_Begin ("class=\"DAT CT COLOR%u\"",Gbl.RowEvenOdd);
      HTM_INPUT_CHECKBOX ("GamCod",HTM_DONT_SUBMIT_ON_CHANGE,
			  "id=\"Gam%u\" value=\"%ld\"%s",
			  NumGame,Games->Lst[NumGame].GamCod,
			  Games->Lst[NumGame].Selected ? " checked=\"checked\"" :
				                         "");
      HTM_TD_End ();

      HTM_TD_Begin ("class=\"DAT RT COLOR%u\"",Gbl.RowEvenOdd);
      HTM_LABEL_Begin ("for=\"Gam%u\"",NumGame);
      HTM_TxtF ("%u:",NumGame + 1);
      HTM_LABEL_End ();
      HTM_TD_End ();

      HTM_TD_Begin ("class=\"DAT LT COLOR%u\"",Gbl.RowEvenOdd);
      HTM_Txt (Game.Title);
      HTM_TD_End ();

      HTM_TR_End ();
     }

   /***** Put button to refresh *****/
   HTM_TR_Begin (NULL);

   HTM_TD_Begin ("colspan=\"3\" class=\"CM\"");
   HTM_BUTTON_Animated_Begin (Txt_Update_results,
			      The_ClassFormLinkInBoxBold[Gbl.Prefs.Theme],
			      NULL);
   Ico_PutCalculateIconWithText (Txt_Update_results);
   HTM_BUTTON_End ();
   HTM_TD_End ();

   HTM_TR_End ();

   /***** End table *****/
   HTM_TABLE_End ();

   /***** End form *****/
   Frm_EndForm ();

   /***** End box *****/
   Box_BoxEnd ();
  }

/*****************************************************************************/
/********************* Show header of my matches results *********************/
/*****************************************************************************/

static void MchRes_ShowHeaderMchResults (Usr_MeOrOther_t MeOrOther)
  {
   extern const char *Txt_User[Usr_NUM_SEXS];
   extern const char *Txt_Match;
   extern const char *Txt_START_END_TIME[Dat_NUM_START_END_TIME];
   extern const char *Txt_Questions;
   extern const char *Txt_Answers;
   extern const char *Txt_Score;
   extern const char *Txt_Grade;
   extern const char *Txt_ANSWERS_non_blank;
   extern const char *Txt_ANSWERS_blank;
   extern const char *Txt_total;
   extern const char *Txt_average;

   /***** First row *****/
   HTM_TR_Begin (NULL);

   HTM_TH (3,2,"CT LINE_BOTTOM",Txt_User[MeOrOther == Usr_ME ? Gbl.Usrs.Me.UsrDat.Sex :
		                                               Usr_SEX_UNKNOWN]);
   HTM_TH (3,1,"LT LINE_BOTTOM",Txt_START_END_TIME[Dat_START_TIME]);
   HTM_TH (3,1,"LT LINE_BOTTOM",Txt_START_END_TIME[Dat_END_TIME  ]);
   HTM_TH (3,1,"LT LINE_BOTTOM",Txt_Match);
   HTM_TH (3,1,"RT LINE_BOTTOM LINE_LEFT",Txt_Questions);
   HTM_TH (1,2,"CT LINE_LEFT",Txt_Answers);
   HTM_TH (1,2,"CT LINE_LEFT",Txt_Score);
   HTM_TH (3,1,"RT LINE_BOTTOM LINE_LEFT",Txt_Grade);
   HTM_TH (3,1,"LINE_BOTTOM LINE_LEFT",NULL);

   HTM_TR_End ();

   /***** Second row *****/
   HTM_TR_Begin (NULL);

   HTM_TH (1,1,"RT LINE_LEFT",Txt_ANSWERS_non_blank);
   HTM_TH (1,1,"RT",Txt_ANSWERS_blank);
   HTM_TH (1,1,"RT LINE_LEFT",Txt_total);
   HTM_TH (1,1,"RT",Txt_average);

   HTM_TR_End ();

   /***** Third row *****/
   HTM_TR_Begin (NULL);

   HTM_TH (1,1,"RT LINE_BOTTOM LINE_LEFT","{-1&le;<em>p<sub>i</sub></em>&le;1}");
   HTM_TH (1,1,"RT LINE_BOTTOM","{<em>p<sub>i</sub></em>=0}");
   HTM_TH (1,1,"RT LINE_BOTTOM LINE_LEFT","<em>&Sigma;p<sub>i</sub></em>");
   HTM_TH (1,1,"RT LINE_BOTTOM","-1&le;<em style=\"text-decoration:overline;\">p</em>&le;1");

   HTM_TR_End ();
  }

/*****************************************************************************/
/******* Build string with list of selected games separated by commas ********/
/******* from list of selected games                                  ********/
/*****************************************************************************/

static void MchRes_BuildGamesSelectedCommas (struct Gam_Games *Games,
                                             char **GamesSelectedCommas)
  {
   size_t MaxLength;
   unsigned NumGame;
   char LongStr[Cns_MAX_DECIMAL_DIGITS_LONG + 1];

   /***** Allocate memory for subquery of games selected *****/
   MaxLength = (size_t) Games->NumSelected * (Cns_MAX_DECIMAL_DIGITS_LONG + 1);
   if ((*GamesSelectedCommas = malloc (MaxLength + 1)) == NULL)
      Lay_NotEnoughMemoryExit ();

   /***** Build subquery with list of selected games *****/
   (*GamesSelectedCommas)[0] = '\0';
   for (NumGame = 0;
	NumGame < Games->Num;
	NumGame++)
      if (Games->Lst[NumGame].Selected)
	{
	 sprintf (LongStr,"%ld",Games->Lst[NumGame].GamCod);
	 if ((*GamesSelectedCommas)[0])
	    Str_Concat (*GamesSelectedCommas,",",MaxLength);
	 Str_Concat (*GamesSelectedCommas,LongStr,MaxLength);
	}
  }

/*****************************************************************************/
/********* Show the matches results of a user in the current course **********/
/*****************************************************************************/

static void MchRes_ShowMchResults (struct Gam_Games *Games,
                                   Usr_MeOrOther_t MeOrOther,
				   long MchCod,	// <= 0 ==> any
				   long GamCod,	// <= 0 ==> any
				   const char *GamesSelectedCommas)
  {
   extern const char *Txt_Result;
   char *MchSubQuery;
   char *GamSubQuery;
   char *HidGamSubQuery;
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   struct UsrData *UsrDat;
   struct MchRes_ICanView ICanView;
   unsigned NumResults;
   unsigned NumResult;
   static unsigned UniqueId = 0;
   Dat_StartEndTime_t StartEndTime;
   char *Id;
   struct MchPrn_Print Print;
   unsigned NumQstsBlank;
   struct Mch_Match Match;
   struct Gam_Game Game;
   double Grade;
   struct MchPrn_NumQuestions NumTotalQsts;
   double TotalScore;
   double TotalGrade;

   /***** Reset total number of questions and total score *****/
   NumTotalQsts.All      =
   NumTotalQsts.NotBlank = 0;
   TotalScore = 0.0;
   TotalGrade = 0.0;

   /***** Set user *****/
   UsrDat = (MeOrOther == Usr_ME) ? &Gbl.Usrs.Me.UsrDat :
				    &Gbl.Usrs.Other.UsrDat;

   /***** Build matches subquery *****/
   if (MchCod > 0)
     {
      if (asprintf (&MchSubQuery," AND mch_results.MchCod=%ld",MchCod) < 0)
	 Lay_NotEnoughMemoryExit ();
     }
   else
     {
      if (asprintf (&MchSubQuery,"%s","") < 0)
	 Lay_NotEnoughMemoryExit ();
     }

   /***** Build games subquery *****/
   if (GamCod > 0)
     {
      if (asprintf (&GamSubQuery," AND mch_matches.GamCod=%ld",GamCod) < 0)
	 Lay_NotEnoughMemoryExit ();
     }
   else if (GamesSelectedCommas)
     {
      if (GamesSelectedCommas[0])
	{
	 if (asprintf (&GamSubQuery," AND mch_matches.GamCod IN (%s)",
		       GamesSelectedCommas) < 0)
	    Lay_NotEnoughMemoryExit ();
	}
      else
	{
	 if (asprintf (&GamSubQuery,"%s","") < 0)
	    Lay_NotEnoughMemoryExit ();
	}
     }
   else
     {
      if (asprintf (&GamSubQuery,"%s","") < 0)
	 Lay_NotEnoughMemoryExit ();
     }

   /***** Subquery: get hidden games?
	  � A student will not be able to see their results in hidden games
	  � A teacher will be able to see results from other users even in hidden games
   *****/
   switch (MeOrOther)
     {
      case Usr_ME:	// A student watching her/his results
         if (asprintf (&HidGamSubQuery," AND gam_games.Hidden='N'") < 0)
	    Lay_NotEnoughMemoryExit ();
	 break;
      default:		// A teacher/admin watching the results of other users
	 if (asprintf (&HidGamSubQuery,"%s","") < 0)
	    Lay_NotEnoughMemoryExit ();
	 break;
     }

   /***** Make database query *****/
   NumResults =
   (unsigned) DB_QuerySELECT (&mysql_res,"can not get matches results",
			      "SELECT mch_results.MchCod"			// row[0]
			      " FROM mch_results,mch_matches,gam_games"
			      " WHERE mch_results.UsrCod=%ld"
			      "%s"	// Match subquery
			      " AND mch_results.MchCod=mch_matches.MchCod"
			      "%s"	// Games subquery
			      " AND mch_matches.GamCod=gam_games.GamCod"
                              "%s"	// Hidden games subquery
			      " AND gam_games.CrsCod=%ld"			// Extra check
			      " ORDER BY mch_matches.Title",
			      UsrDat->UsrCod,
			      MchSubQuery,
			      GamSubQuery,
			      HidGamSubQuery,
			      Gbl.Hierarchy.Crs.CrsCod);
   free (HidGamSubQuery);
   free (GamSubQuery);
   free (MchSubQuery);

   /***** Show user's data *****/
   HTM_TR_Begin (NULL);
   Usr_ShowTableCellWithUsrData (UsrDat,NumResults);

   /***** Get and print matches results *****/
   if (NumResults)
     {
      for (NumResult = 0;
	   NumResult < NumResults;
	   NumResult++)
	{
	 row = mysql_fetch_row (mysql_res);

	 /* Get match code (row[0]) */
         MchPrn_ResetPrint (&Print);
	 if ((Print.MchCod = Str_ConvertStrCodToLongCod (row[0])) < 0)
	    Lay_ShowErrorAndExit ("Wrong code of match.");

	 /* Get match result data */
	 Print.UsrCod = UsrDat->UsrCod;
         MchPrn_GetMatchPrintDataByMchCodAndUsrCod (&Print);

	 /* Get data of match and game */
	 Match.MchCod = Print.MchCod;
	 Mch_GetDataOfMatchByCod (&Match);
	 Game.GamCod = Match.GamCod;
	 Gam_GetDataOfGameByCod (&Game);

         /* Check if I can view this match result and score */
	 MchRes_CheckIfICanSeeMatchResult (&Game,&Match,UsrDat->UsrCod,&ICanView);

	 if (NumResult)
	    HTM_TR_Begin (NULL);

	 /* Write start/end times */
	 for (StartEndTime  = (Dat_StartEndTime_t) 0;
	      StartEndTime <= (Dat_StartEndTime_t) (Dat_NUM_START_END_TIME - 1);
	      StartEndTime++)
	   {
	    UniqueId++;
	    if (asprintf (&Id,"mch_res_time_%u_%u",(unsigned) StartEndTime,UniqueId) < 0)
	       Lay_NotEnoughMemoryExit ();
	    HTM_TD_Begin ("id =\"%s\" class=\"DAT LT COLOR%u\"",
			  Id,Gbl.RowEvenOdd);
	    Dat_WriteLocalDateHMSFromUTC (Id,Print.TimeUTC[StartEndTime],
					  Gbl.Prefs.DateFormat,Dat_SEPARATOR_BREAK,
					  true,true,false,0x7);
	    HTM_TD_End ();
	    free (Id);
	   }

	 /* Write match title */
	 HTM_TD_Begin ("class=\"DAT LT COLOR%u\"",Gbl.RowEvenOdd);
	 HTM_Txt (Match.Title);
	 HTM_TD_End ();

	 /* Accumulate questions and score */
	 if (ICanView.Score)
	   {
	    NumTotalQsts.All      += Print.NumQsts.All;
            NumTotalQsts.NotBlank += Print.NumQsts.NotBlank;
            TotalScore            += Print.Score;
	   }

	 /* Write number of questions */
	 HTM_TD_Begin ("class=\"DAT RT LINE_LEFT COLOR%u\"",Gbl.RowEvenOdd);
	 if (ICanView.Score)
	    HTM_Unsigned (Print.NumQsts.All);
	 else
            Ico_PutIconNotVisible ();
         HTM_TD_End ();

	 /* Write number of non-blank answers */
	 HTM_TD_Begin ("class=\"DAT RT LINE_LEFT COLOR%u\"",Gbl.RowEvenOdd);
	 if (ICanView.Score)
	   {
	    if (Print.NumQsts.NotBlank)
	       HTM_Unsigned (Print.NumQsts.NotBlank);
	    else
	       HTM_Light0 ();
	   }
	 else
            Ico_PutIconNotVisible ();
	 HTM_TD_End ();

	 /* Write number of blank answers */
	 HTM_TD_Begin ("class=\"DAT RT COLOR%u\"",Gbl.RowEvenOdd);
	 NumQstsBlank = Print.NumQsts.All - Print.NumQsts.NotBlank;
	 if (ICanView.Score)
	   {
	    if (NumQstsBlank)
	       HTM_Unsigned (NumQstsBlank);
	    else
	       HTM_Light0 ();
	   }
	 else
            Ico_PutIconNotVisible ();
	 HTM_TD_End ();

	 /* Write score */
	 HTM_TD_Begin ("class=\"DAT RT LINE_LEFT COLOR%u\"",Gbl.RowEvenOdd);
	 if (ICanView.Score)
	   {
	    HTM_Double2Decimals (Print.Score);
	    HTM_Txt ("/");
	    HTM_Unsigned (Print.NumQsts.All);
	   }
	 else
            Ico_PutIconNotVisible ();
	 HTM_TD_End ();

	 /* Write average score per question */
	 HTM_TD_Begin ("class=\"DAT RT COLOR%u\"",Gbl.RowEvenOdd);
	 if (ICanView.Score)
	    HTM_Double2Decimals (Print.NumQsts.All ? Print.Score /
					             (double) Print.NumQsts.All :
					             0.0);
	 else
            Ico_PutIconNotVisible ();
	 HTM_TD_End ();

	 /* Write grade over maximum grade */
	 HTM_TD_Begin ("class=\"DAT RT LINE_LEFT COLOR%u\"",Gbl.RowEvenOdd);
	 if (ICanView.Score)
	   {
            Grade = TstPrn_ComputeGrade (Print.NumQsts.All,Print.Score,Game.MaxGrade);
	    TstPrn_ShowGrade (Grade,Game.MaxGrade);
	    TotalGrade += Grade;
	   }
	 else
            Ico_PutIconNotVisible ();
	 HTM_TD_End ();

	 /* Link to show this result */
	 HTM_TD_Begin ("class=\"RT LINE_LEFT COLOR%u\"",Gbl.RowEvenOdd);
	 if (ICanView.Result)
	   {
	    Games->GamCod         = Match.GamCod;
	    Games->MchCod.Current = Match.MchCod;
	    switch (MeOrOther)
	      {
	       case Usr_ME:
		  Frm_StartForm (ActSeeOneMchResMe);
		  Mch_PutParamsEdit (Games);
		  break;
	       case Usr_OTHER:
		  Frm_StartForm (ActSeeOneMchResOth);
		  Mch_PutParamsEdit (Games);
		  Usr_PutParamOtherUsrCodEncrypted (Gbl.Usrs.Other.UsrDat.EnUsrCod);
		  break;
	      }
	    Ico_PutIconLink ("tasks.svg",Txt_Result);
	    Frm_EndForm ();
	   }
	 else
            Ico_PutIconNotVisible ();
	 HTM_TD_End ();

	 HTM_TR_End ();
	}

      /***** Write totals for this user *****/
      MchRes_ShowMchResultsSummaryRow (NumResults,
				       &NumTotalQsts,
				       TotalScore,
				       TotalGrade);
     }
   else
     {
      /* Columns for dates and match */
      HTM_TD_Begin ("colspan=\"3\" class=\"LINE_BOTTOM COLOR%u\"",Gbl.RowEvenOdd);
      HTM_TD_End ();

      /* Column for questions */
      HTM_TD_Begin ("class=\"LINE_BOTTOM LINE_LEFT COLOR%u\"",Gbl.RowEvenOdd);
      HTM_TD_End ();

      /* Columns for answers */
      HTM_TD_Begin ("colspan=\"2\" class=\"LINE_BOTTOM LINE_LEFT COLOR%u\"",Gbl.RowEvenOdd);
      HTM_TD_End ();

      /* Columns for score */
      HTM_TD_Begin ("colspan=\"2\" class=\"LINE_BOTTOM LINE_LEFT COLOR%u\"",Gbl.RowEvenOdd);
      HTM_TD_End ();

      /* Column for grade */
      HTM_TD_Begin ("class=\"LINE_BOTTOM LINE_LEFT COLOR%u\"",Gbl.RowEvenOdd);
      HTM_TD_End ();

      /* Column for link to show the result */
      HTM_TD_Begin ("class=\"LINE_BOTTOM LINE_LEFT COLOR%u\"",Gbl.RowEvenOdd);
      HTM_TD_End ();

      HTM_TR_End ();
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   Gbl.RowEvenOdd = 1 - Gbl.RowEvenOdd;
  }

/*****************************************************************************/
/************** Show row with summary of user's matches results **************/
/*****************************************************************************/

static void MchRes_ShowMchResultsSummaryRow (unsigned NumResults,
                                             struct MchPrn_NumQuestions *NumTotalQsts,
                                             double TotalScore,
					     double TotalGrade)
  {
   extern const char *Txt_Matches;

   /***** Start row *****/
   HTM_TR_Begin (NULL);

   /***** Row title *****/
   HTM_TD_Begin ("colspan=\"3\" class=\"DAT_N RM LINE_TOP LINE_BOTTOM COLOR%u\"",Gbl.RowEvenOdd);
   HTM_TxtColonNBSP (Txt_Matches);
   HTM_Unsigned (NumResults);
   HTM_TD_End ();

   /***** Write total number of questions *****/
   HTM_TD_Begin ("class=\"DAT_N RM LINE_TOP LINE_BOTTOM LINE_LEFT COLOR%u\"",Gbl.RowEvenOdd);
   if (NumResults)
      HTM_Unsigned (NumTotalQsts->All);
   HTM_TD_End ();

   /***** Write total number of non-blank answers *****/
   HTM_TD_Begin ("class=\"DAT_N RM LINE_TOP LINE_BOTTOM LINE_LEFT COLOR%u\"",Gbl.RowEvenOdd);
   if (NumResults)
      HTM_Unsigned (NumTotalQsts->NotBlank);
   HTM_TD_End ();

   /***** Write total number of blank answers *****/
   HTM_TD_Begin ("class=\"DAT_N RM LINE_TOP LINE_BOTTOM COLOR%u\"",Gbl.RowEvenOdd);
   if (NumResults)
      HTM_Unsigned (NumTotalQsts->All - NumTotalQsts->NotBlank);
   HTM_TD_End ();

   /***** Write total score *****/
   HTM_TD_Begin ("class=\"DAT_N RM LINE_TOP LINE_BOTTOM LINE_LEFT COLOR%u\"",Gbl.RowEvenOdd);
   HTM_Double2Decimals (TotalScore);
   HTM_Txt ("/");
   HTM_Unsigned (NumTotalQsts->All);
   HTM_TD_End ();

   /***** Write average score per question *****/
   HTM_TD_Begin ("class=\"DAT_N RM LINE_TOP LINE_BOTTOM COLOR%u\"",Gbl.RowEvenOdd);
   HTM_Double2Decimals (NumTotalQsts->All ? TotalScore /
	                                    (double) NumTotalQsts->All :
			                    0.0);
   HTM_TD_End ();

   /***** Write total grade *****/
   HTM_TD_Begin ("class=\"DAT_N RM LINE_TOP LINE_BOTTOM LINE_LEFT COLOR%u\"",Gbl.RowEvenOdd);
   HTM_Double2Decimals (TotalGrade);
   HTM_TD_End ();

   /***** Last cell *****/
   HTM_TD_Begin ("class=\"DAT_N LINE_TOP LINE_BOTTOM LINE_LEFT COLOR%u\"",Gbl.RowEvenOdd);
   HTM_TD_End ();

   /***** End row *****/
   HTM_TR_End ();
  }

/*****************************************************************************/
/******************* Show one match result of another user *******************/
/*****************************************************************************/

void MchRes_ShowOneMchResult (void)
  {
   extern const char *Hlp_ASSESSMENT_Games_results;
   extern const char *Txt_The_user_does_not_exist;
   extern const char *Txt_ROLES_SINGUL_Abc[Rol_NUM_ROLES][Usr_NUM_SEXS];
   extern const char *Txt_START_END_TIME[Dat_NUM_START_END_TIME];
   extern const char *Txt_Questions;
   extern const char *Txt_Answers;
   extern const char *Txt_Score;
   extern const char *Txt_Grade;
   extern const char *Txt_Tags;
   struct Gam_Games Games;
   struct Gam_Game Game;
   struct Mch_Match Match;
   Usr_MeOrOther_t MeOrOther;
   struct UsrData *UsrDat;
   Dat_StartEndTime_t StartEndTime;
   char *Id;
   struct MchPrn_Print Print;
   bool ShowPhoto;
   char PhotoURL[PATH_MAX + 1];
   struct MchRes_ICanView ICanView;

   /***** Reset games context *****/
   Gam_ResetGames (&Games);

   /***** Reset game and match *****/
   Gam_ResetGame (&Game);
   Mch_ResetMatch (&Match);

   /***** Get and check parameters *****/
   Mch_GetAndCheckParameters (&Games,&Game,&Match);

   /***** Pointer to user's data *****/
   MeOrOther = (Gbl.Action.Act == ActSeeOneMchResMe) ? Usr_ME :
	                                               Usr_OTHER;
   switch (MeOrOther)
     {
      case Usr_ME:
	 UsrDat = &Gbl.Usrs.Me.UsrDat;
	 break;
      case Usr_OTHER:
      default:
	 UsrDat = &Gbl.Usrs.Other.UsrDat;
         Usr_GetParamOtherUsrCodEncrypted (UsrDat);
	 break;
     }

   /***** Get match result data *****/
   Print.MchCod = Match.MchCod;
   Print.UsrCod = UsrDat->UsrCod;
   MchPrn_GetMatchPrintDataByMchCodAndUsrCod (&Print);

   /***** Check if I can view this match result and score *****/
   MchRes_CheckIfICanSeeMatchResult (&Game,&Match,UsrDat->UsrCod,&ICanView);

   if (ICanView.Result)	// I am allowed to view this match result
     {
      /***** Get questions and user's answers of the match result from database *****/
      Mch_GetMatchQuestionsFromDB (&Print);

      /***** Begin box *****/
      Box_BoxBegin (NULL,Match.Title,
                    NULL,NULL,
                    Hlp_ASSESSMENT_Games_results,Box_NOT_CLOSABLE);
      Lay_WriteHeaderClassPhoto (false,false,
				 Gbl.Hierarchy.Ins.InsCod,
				 Gbl.Hierarchy.Deg.DegCod,
				 Gbl.Hierarchy.Crs.CrsCod);

      /***** Begin table *****/
      HTM_TABLE_BeginWideMarginPadding (10);

      /***** User *****/
      /* Get data of the user who answer the match */
      if (!Usr_ChkUsrCodAndGetAllUsrDataFromUsrCod (UsrDat,Usr_DONT_GET_PREFS))
	 Lay_ShowErrorAndExit (Txt_The_user_does_not_exist);
      if (!Usr_CheckIfICanViewTstExaMchResult (UsrDat))
         Lay_NoPermissionExit ();

      /* User */
      HTM_TR_Begin (NULL);

      HTM_TD_Begin ("class=\"DAT_N RT\"");
      HTM_TxtColon (Txt_ROLES_SINGUL_Abc[UsrDat->Roles.InCurrentCrs.Role][UsrDat->Sex]);
      HTM_TD_End ();

      HTM_TD_Begin ("class=\"DAT LB\"");
      ID_WriteUsrIDs (UsrDat,NULL);
      HTM_TxtF ("&nbsp;%s",UsrDat->Surname1);
      if (UsrDat->Surname2[0])
	 HTM_TxtF ("&nbsp;%s",UsrDat->Surname2);
      if (UsrDat->FrstName[0])
	 HTM_TxtF (", %s",UsrDat->FrstName);
      HTM_BR ();
      ShowPhoto = Pho_ShowingUsrPhotoIsAllowed (UsrDat,PhotoURL);
      Pho_ShowUsrPhoto (UsrDat,ShowPhoto ? PhotoURL :
					   NULL,
			"PHOTO45x60",Pho_ZOOM,false);
      HTM_TD_End ();

      HTM_TR_End ();

      /***** Start/end time (for user in this match) *****/
      for (StartEndTime  = (Dat_StartEndTime_t) 0;
	   StartEndTime <= (Dat_StartEndTime_t) (Dat_NUM_START_END_TIME - 1);
	   StartEndTime++)
	{
	 HTM_TR_Begin (NULL);

	 HTM_TD_Begin ("class=\"DAT_N RT\"");
	 HTM_TxtColon (Txt_START_END_TIME[StartEndTime]);
	 HTM_TD_End ();

	 if (asprintf (&Id,"match_%u",(unsigned) StartEndTime) < 0)
	    Lay_NotEnoughMemoryExit ();
	 HTM_TD_Begin ("id=\"%s\" class=\"DAT LB\"",Id);
	 Dat_WriteLocalDateHMSFromUTC (Id,Print.TimeUTC[StartEndTime],
				       Gbl.Prefs.DateFormat,Dat_SEPARATOR_COMMA,
				       true,true,true,0x7);
	 HTM_TD_End ();
         free (Id);

	 HTM_TR_End ();
	}

      /***** Number of questions *****/
      HTM_TR_Begin (NULL);

      HTM_TD_Begin ("class=\"DAT_N RT\"");
      HTM_TxtColon (Txt_Questions);
      HTM_TD_End ();

      HTM_TD_Begin ("class=\"DAT LB\"");
      HTM_Unsigned (Print.NumQsts.All);
      HTM_TD_End ();

      HTM_TR_End ();

      /***** Number of answers *****/
      HTM_TR_Begin (NULL);

      HTM_TD_Begin ("class=\"DAT_N RT\"");
      HTM_TxtColon (Txt_Answers);
      HTM_TD_End ();

      HTM_TD_Begin ("class=\"DAT LB\"");
      HTM_Unsigned (Print.NumQsts.NotBlank);
      HTM_TD_End ();

      HTM_TR_End ();

      /***** Score *****/
      HTM_TR_Begin (NULL);

      HTM_TD_Begin ("class=\"DAT_N RT\"");
      HTM_TxtColon (Txt_Score);
      HTM_TD_End ();

      HTM_TD_Begin ("class=\"DAT LB\"");
      if (ICanView.Score)
	{
         HTM_STRONG_Begin ();
         HTM_Double2Decimals (Print.Score);
	 HTM_Txt ("/");
	 HTM_Unsigned (Print.NumQsts.All);
         HTM_STRONG_End ();
	}
      else
         Ico_PutIconNotVisible ();
      HTM_TD_End ();

      HTM_TR_End ();

      /***** Grade *****/
      HTM_TR_Begin (NULL);

      HTM_TD_Begin ("class=\"DAT_N RT\"");
      HTM_TxtColon (Txt_Grade);
      HTM_TD_End ();

      HTM_TD_Begin ("class=\"DAT LB\"");
      if (ICanView.Score)
	{
         HTM_STRONG_Begin ();
         TstPrn_ComputeAndShowGrade (Print.NumQsts.All,Print.Score,Game.MaxGrade);
         HTM_STRONG_End ();
	}
      else
         Ico_PutIconNotVisible ();
      HTM_TD_End ();

      HTM_TR_End ();

      /***** Tags present in this result *****/
      HTM_TR_Begin (NULL);

      HTM_TD_Begin ("class=\"DAT_N RT\"");
      HTM_TxtColon (Txt_Tags);
      HTM_TD_End ();

      HTM_TD_Begin ("class=\"DAT LB\"");
      Gam_ShowTstTagsPresentInAGame (Match.GamCod);
      HTM_TD_End ();

      HTM_TR_End ();

      /***** Write answers and solutions *****/
      TstPrn_ShowPrintAnswers (UsrDat,
                               Print.NumQsts.All,
                               Print.PrintedQuestions,
                               Print.TimeUTC,
                               Game.Visibility);

      /***** End table *****/
      HTM_TABLE_End ();

      /***** End box *****/
      Box_BoxEnd ();
     }
   else	// I am not allowed to view this match result
      Lay_NoPermissionExit ();
  }

/*****************************************************************************/
/********************** Get if I can see match result ************************/
/*****************************************************************************/

static void MchRes_CheckIfICanSeeMatchResult (const struct Gam_Game *Game,
                                              const struct Mch_Match *Match,
                                              long UsrCod,
                                              struct MchRes_ICanView *ICanView)
  {
   bool ItsMe;

   /***** Check if I can view print result and score *****/
   switch (Gbl.Usrs.Me.Role.Logged)
     {
      case Rol_STD:
	 // Depends on visibility of game and result (eye icons)
	 ItsMe = Usr_ItsMe (UsrCod);
	 ICanView->Result = (ItsMe &&				// The result is mine
			     !Game->Hidden &&			// The game is visible
			     Match->Status.ShowUsrResults);	// The results of the match are visible to users
	 // Whether I belong or not to groups of match is not checked here...
	 // ...because I should be able to see old matches made in old groups to which I belonged

	 if (ICanView->Result)
	    // Depends on 5 visibility icons associated to game
	    ICanView->Score = TstVis_IsVisibleTotalScore (Game->Visibility);
	 else
	    ICanView->Score = false;
	 break;
      case Rol_NET:
      case Rol_TCH:
      case Rol_DEG_ADM:
      case Rol_CTR_ADM:
      case Rol_INS_ADM:
      case Rol_SYS_ADM:
	 ICanView->Result =
	 ICanView->Score  = true;
	 break;
      default:
	 ICanView->Result =
	 ICanView->Score  = false;
	 break;
     }
  }
