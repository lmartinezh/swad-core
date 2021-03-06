// swad_course_config.c: configuration of current course

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
#include <stdbool.h>		// For boolean type
#include <stddef.h>		// For NULL
#include <stdio.h>		// For asprintf
#include <stdlib.h>		// For free
#include <string.h>		// For string functions

#include "swad_database.h"
#include "swad_form.h"
#include "swad_global.h"
#include "swad_hierarchy.h"
#include "swad_hierarchy_config.h"
#include "swad_HTML.h"
#include "swad_indicator.h"
#include "swad_logo.h"
#include "swad_role.h"

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/*************************** Public constants ********************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Private types *********************************/
/*****************************************************************************/

/*****************************************************************************/
/**************************** Private variables ******************************/
/*****************************************************************************/

/*****************************************************************************/
/**************************** Private prototypes *****************************/
/*****************************************************************************/

static void CrsCfg_PutIconToPrint (__attribute__((unused)) void *Args);
static void CrsCfg_Title (bool PutLink);
static void CrsCfg_Degree (bool PrintView,bool PutForm);
static void CrsCfg_FullName (bool PutForm);
static void CrsCfg_ShrtName (bool PutForm);
static void CrsCfg_Year (bool PutForm);
static void CrsCfg_InstitutionalCode (bool PutForm);
static void CrsCfg_InternalCode (void);
static void CrsCfg_Shortcut (bool PrintView);
static void CrsCfg_QR (void);
static void CrsCfg_Indicators (void);

static void CrsCfg_UpdateCrsDegDB (long CrsCod,long DegCod);

/*****************************************************************************/
/***************** Configuration of the current course ***********************/
/*****************************************************************************/

void CrsCfg_Configuration (bool PrintView)
  {
   extern const char *Hlp_COURSE_Information;
   bool PutLink;
   bool PutFormDeg;
   bool PutFormName;
   bool PutFormYear;
   bool PutFormInsCod;

   /***** Trivial check *****/
   if (Gbl.Hierarchy.Crs.CrsCod <= 0)	// No course selected
      return;

   /***** Initializations *****/
   PutLink       = !PrintView && Gbl.Hierarchy.Deg.WWW[0];
   PutFormDeg    = !PrintView && Gbl.Usrs.Me.Role.Logged >= Rol_CTR_ADM;
   PutFormName   = !PrintView && Gbl.Usrs.Me.Role.Logged >= Rol_DEG_ADM;
   PutFormYear   =
   PutFormInsCod = !PrintView && Gbl.Usrs.Me.Role.Logged >= Rol_TCH;

   /***** Contextual menu *****/
   if (!PrintView)
      if (Gbl.Usrs.Me.Role.Logged == Rol_GST ||
	  Gbl.Usrs.Me.Role.Logged == Rol_USR)
	{
         Mnu_ContextMenuBegin ();
         Enr_PutLinkToRequestSignUp ();	// Request enrolment in the current course
         Mnu_ContextMenuEnd ();
	}

   /***** Begin box *****/
   if (PrintView)
      Box_BoxBegin (NULL,NULL,
                    NULL,NULL,
		    NULL,Box_NOT_CLOSABLE);
   else
      Box_BoxBegin (NULL,NULL,
                    CrsCfg_PutIconToPrint,NULL,
		    Hlp_COURSE_Information,Box_NOT_CLOSABLE);

   /***** Title *****/
   CrsCfg_Title (PutLink);

   /**************************** Left part ***********************************/
   HTM_DIV_Begin ("class=\"HIE_CFG_LEFT HIE_CFG_WIDTH\"");

   /***** Begin table *****/
   HTM_TABLE_BeginWidePadding (2);

   /***** Degree *****/
   CrsCfg_Degree (PrintView,PutFormDeg);

   /***** Course name *****/
   CrsCfg_FullName (PutFormName);
   CrsCfg_ShrtName (PutFormName);

   /***** Course year *****/
   CrsCfg_Year (PutFormYear);

   if (!PrintView)
     {
      /***** Institutional code of the course *****/
      CrsCfg_InstitutionalCode (PutFormInsCod);

      /***** Internal code of the course *****/
      CrsCfg_InternalCode ();
     }

   /***** Shortcut to the couse *****/
   CrsCfg_Shortcut (PrintView);

   if (PrintView)
      /***** QR code with link to the course *****/
      CrsCfg_QR ();
   else
     {
      /***** Number of users *****/
      HieCfg_NumUsrsInCrss (Hie_Lvl_CRS,Gbl.Hierarchy.Crs.CrsCod,Rol_TCH);
      HieCfg_NumUsrsInCrss (Hie_Lvl_CRS,Gbl.Hierarchy.Crs.CrsCod,Rol_NET);
      HieCfg_NumUsrsInCrss (Hie_Lvl_CRS,Gbl.Hierarchy.Crs.CrsCod,Rol_STD);
      HieCfg_NumUsrsInCrss (Hie_Lvl_CRS,Gbl.Hierarchy.Crs.CrsCod,Rol_UNK);

      /***** Indicators *****/
      CrsCfg_Indicators ();
     }

   /***** End table *****/
   HTM_TABLE_End ();

   /***** End of left part *****/
   HTM_DIV_End ();

   /***** End box *****/
   Box_BoxEnd ();
  }

/*****************************************************************************/
/************* Put icon to print the configuration of a course ***************/
/*****************************************************************************/

static void CrsCfg_PutIconToPrint (__attribute__((unused)) void *Args)
  {
   Ico_PutContextualIconToPrint (ActPrnCrsInf,
				 NULL,NULL);
  }

/*****************************************************************************/
/***************** Print configuration of the current course *****************/
/*****************************************************************************/

void CrsCfg_PrintConfiguration (void)
  {
   CrsCfg_Configuration (true);
  }

/*****************************************************************************/
/******************** Show title in course configuration *********************/
/*****************************************************************************/

static void CrsCfg_Title (bool PutLink)
  {
   HieCfg_Title (PutLink,
		    Hie_Lvl_DEG,				// Logo scope
		    Gbl.Hierarchy.Deg.DegCod,		// Logo code
                    Gbl.Hierarchy.Deg.ShrtName,		// Logo short name
		    Gbl.Hierarchy.Deg.FullName,		// Logo full name
		    Gbl.Hierarchy.Deg.WWW,		// Logo www
		    Gbl.Hierarchy.Crs.FullName);	// Text full name
  }

/*****************************************************************************/
/******************** Show degree in course configuration ********************/
/*****************************************************************************/

static void CrsCfg_Degree (bool PrintView,bool PutForm)
  {
   extern const char *Txt_Degree;
   unsigned NumDeg;

   /***** Degree *****/
   HTM_TR_Begin (NULL);

   /* Label */
   Frm_LabelColumn ("RT",PutForm ? "OthDegCod" :
	                           NULL,
		    Txt_Degree);

   /* Data */
   HTM_TD_Begin ("class=\"DAT LB\"");
   if (PutForm)
     {
      /* Get list of degrees of the current centre */
      Deg_GetListDegsInCurrentCtr ();

      /* Put form to select degree */
      Frm_StartForm (ActChgCrsDegCfg);
      HTM_SELECT_Begin (HTM_SUBMIT_ON_CHANGE,
			"id=\"OthDegCod\" name=\"OthDegCod\""
			" class=\"INPUT_SHORT_NAME\"");
      for (NumDeg = 0;
	   NumDeg < Gbl.Hierarchy.Degs.Num;
	   NumDeg++)
	 HTM_OPTION (HTM_Type_LONG,&Gbl.Hierarchy.Degs.Lst[NumDeg].DegCod,
		     Gbl.Hierarchy.Degs.Lst[NumDeg].DegCod == Gbl.Hierarchy.Deg.DegCod,false,
		     "%s",Gbl.Hierarchy.Degs.Lst[NumDeg].ShrtName);
      HTM_SELECT_End ();
      Frm_EndForm ();

      /* Free list of degrees of the current centre */
      Deg_FreeListDegs (&Gbl.Hierarchy.Degs);
     }
   else	// I can not move course to another degree
     {
      if (!PrintView)
	{
         Frm_StartFormGoTo (ActSeeDegInf);
         Deg_PutParamDegCod (Gbl.Hierarchy.Deg.DegCod);
	 HTM_BUTTON_SUBMIT_Begin (Hie_BuildGoToMsg (Gbl.Hierarchy.Deg.ShrtName),
				  "BT_LINK LT DAT",NULL);
         Hie_FreeGoToMsg ();
	}
      Lgo_DrawLogo (Hie_Lvl_DEG,Gbl.Hierarchy.Deg.DegCod,Gbl.Hierarchy.Deg.ShrtName,
		    20,"LM",true);
      HTM_NBSP ();
      HTM_Txt (Gbl.Hierarchy.Deg.FullName);
      if (!PrintView)
	{
	 HTM_BUTTON_End ();
	 Frm_EndForm ();
	}
     }
   HTM_TD_End ();

   HTM_TR_End ();
  }

/*****************************************************************************/
/************** Show course full name in course configuration ****************/
/*****************************************************************************/

static void CrsCfg_FullName (bool PutForm)
  {
   extern const char *Txt_Course;

   HieCfg_FullName (PutForm,Txt_Course,ActRenCrsFulCfg,
		       Gbl.Hierarchy.Crs.FullName);
  }

/*****************************************************************************/
/************** Show course short name in course configuration ***************/
/*****************************************************************************/

static void CrsCfg_ShrtName (bool PutForm)
  {
   HieCfg_ShrtName (PutForm,ActRenCrsShoCfg,Gbl.Hierarchy.Crs.ShrtName);
  }

/*****************************************************************************/
/***************** Show course year in course configuration ******************/
/*****************************************************************************/

static void CrsCfg_Year (bool PutForm)
  {
   extern const char *Txt_Year_OF_A_DEGREE;
   extern const char *Txt_YEAR_OF_DEGREE[1 + Deg_MAX_YEARS_PER_DEGREE];
   extern const char *Txt_Not_applicable;
   unsigned Year;

   /***** Academic year *****/
   HTM_TR_Begin (NULL);

   /* Label */
   Frm_LabelColumn ("RT",PutForm ? "OthCrsYear" :
	                           NULL,
		    Txt_Year_OF_A_DEGREE);

   /* Data */
   HTM_TD_Begin ("class=\"DAT LB\"");
   if (PutForm)
     {
      Frm_StartForm (ActChgCrsYeaCfg);
      HTM_SELECT_Begin (HTM_SUBMIT_ON_CHANGE,
			"id=\"OthCrsYear\" name=\"OthCrsYear\"");
      for (Year = 0;
	   Year <= Deg_MAX_YEARS_PER_DEGREE;
           Year++)
	 HTM_OPTION (HTM_Type_UNSIGNED,&Year,
		     Year == Gbl.Hierarchy.Crs.Year,false,
		     "%s",Txt_YEAR_OF_DEGREE[Year]);
      HTM_SELECT_End ();
      Frm_EndForm ();
     }
   else
      HTM_Txt (Gbl.Hierarchy.Crs.Year ? Txt_YEAR_OF_DEGREE[Gbl.Hierarchy.Crs.Year] :
	                                Txt_Not_applicable);
   HTM_TD_End ();

   HTM_TR_End ();
  }

/*****************************************************************************/
/************* Show institutional code in course configuration ***************/
/*****************************************************************************/

static void CrsCfg_InstitutionalCode (bool PutForm)
  {
   extern const char *Txt_Institutional_code;

   /***** Institutional course code *****/
   HTM_TR_Begin (NULL);

   /* Label */
   Frm_LabelColumn ("RT",PutForm ? "InsCrsCod" :
	                           NULL,
		    Txt_Institutional_code);

   /* Data */
   HTM_TD_Begin ("class=\"DAT LB\"");
   if (PutForm)
     {
      Frm_StartForm (ActChgInsCrsCodCfg);
      HTM_INPUT_TEXT ("InsCrsCod",Crs_MAX_CHARS_INSTITUTIONAL_CRS_COD,
		      Gbl.Hierarchy.Crs.InstitutionalCrsCod,
		      HTM_SUBMIT_ON_CHANGE,
		      "id=\"InsCrsCod\" size=\"%u\" class=\"INPUT_INS_CODE\"",
		      Crs_MAX_CHARS_INSTITUTIONAL_CRS_COD);
      Frm_EndForm ();
     }
   else
      HTM_Txt (Gbl.Hierarchy.Crs.InstitutionalCrsCod);
   HTM_TD_End ();

   HTM_TR_End ();
  }

/*****************************************************************************/
/**************** Show internal code in course configuration *****************/
/*****************************************************************************/

static void CrsCfg_InternalCode (void)
  {
   extern const char *Txt_Internal_code;

   /***** Internal course code *****/
   HTM_TR_Begin (NULL);

   /* Label */
   Frm_LabelColumn ("RT",NULL,Txt_Internal_code);

   /* Data */
   HTM_TD_Begin ("class=\"DAT LB\"");
   HTM_Long (Gbl.Hierarchy.Crs.CrsCod);
   HTM_TD_End ();

   HTM_TR_End ();
  }

/*****************************************************************************/
/*************** Show course shortcut in course configuration ****************/
/*****************************************************************************/

static void CrsCfg_Shortcut (bool PrintView)
  {
   HieCfg_Shortcut (PrintView,"crs",Gbl.Hierarchy.Crs.CrsCod);
  }

/*****************************************************************************/
/****************** Show course QR in course configuration *******************/
/*****************************************************************************/

static void CrsCfg_QR (void)
  {
   HieCfg_QR ("crs",Gbl.Hierarchy.Crs.CrsCod);
  }

/*****************************************************************************/
/****************** Show indicators in course configuration ******************/
/*****************************************************************************/

static void CrsCfg_Indicators (void)
  {
   extern const char *Txt_Indicators;
   extern const char *Txt_of_PART_OF_A_TOTAL;
   struct Ind_IndicatorsCrs IndicatorsCrs;
   int NumIndicatorsFromDB = Ind_GetNumIndicatorsCrsFromDB (Gbl.Hierarchy.Crs.CrsCod);
   char *Title;

   /***** Compute indicators ******/
   Ind_ComputeAndStoreIndicatorsCrs (Gbl.Hierarchy.Crs.CrsCod,
				     NumIndicatorsFromDB,&IndicatorsCrs);

   /***** Number of indicators *****/
   HTM_TR_Begin (NULL);

   /* Label */
   Frm_LabelColumn ("RT",NULL,Txt_Indicators);

   /* Data */
   HTM_TD_Begin ("class=\"LB\"");
   Frm_StartForm (ActReqStaCrs);
   if (asprintf (&Title,"%u %s %u",
	         IndicatorsCrs.NumIndicators,
	         Txt_of_PART_OF_A_TOTAL,Ind_NUM_INDICATORS) < 0)
      Lay_NotEnoughMemoryExit ();
   HTM_BUTTON_SUBMIT_Begin (Title,"BT_LINK DAT",NULL);
   HTM_TxtF ("%s&nbsp;",Title);
   Ico_PutIcon ((IndicatorsCrs.NumIndicators == Ind_NUM_INDICATORS) ? "check-circle.svg" :
								      "exclamation-triangle.svg",
		Title,"ICO16x16");
   HTM_BUTTON_End ();
   free (Title);
   Frm_EndForm ();
   HTM_TD_End ();

   HTM_TR_End ();
  }

/*****************************************************************************/
/***************** Change the degree of the current course *******************/
/*****************************************************************************/

void CrsCfg_ChangeCrsDeg (void)
  {
   extern const char *Txt_In_the_year_X_of_the_degree_Y_already_existed_a_course_with_the_name_Z;
   extern const char *Txt_YEAR_OF_DEGREE[1 + Deg_MAX_YEARS_PER_DEGREE];
   extern const char *Txt_The_course_X_has_been_moved_to_the_degree_Y;
   struct Deg_Degree NewDeg;

   /***** Get parameter with degree code *****/
   NewDeg.DegCod = Deg_GetAndCheckParamOtherDegCod (1);

   /***** Check if degree has changed *****/
   if (NewDeg.DegCod != Gbl.Hierarchy.Crs.DegCod)
     {
      /***** Get data of new degree *****/
      Deg_GetDataOfDegreeByCod (&NewDeg);

      /***** If name of course was in database in the new degree... *****/
      if (Crs_CheckIfCrsNameExistsInYearOfDeg ("ShortName",Gbl.Hierarchy.Crs.ShrtName,-1L,
                                               NewDeg.DegCod,Gbl.Hierarchy.Crs.Year))
	 Ale_CreateAlert (Ale_WARNING,NULL,
	                  Txt_In_the_year_X_of_the_degree_Y_already_existed_a_course_with_the_name_Z,
		          Txt_YEAR_OF_DEGREE[Gbl.Hierarchy.Crs.Year],
			  NewDeg.FullName,
		          Gbl.Hierarchy.Crs.ShrtName);
      else if (Crs_CheckIfCrsNameExistsInYearOfDeg ("FullName",Gbl.Hierarchy.Crs.FullName,-1L,
                                                    NewDeg.DegCod,Gbl.Hierarchy.Crs.Year))
	 Ale_CreateAlert (Ale_WARNING,NULL,
	                  Txt_In_the_year_X_of_the_degree_Y_already_existed_a_course_with_the_name_Z,
		          Txt_YEAR_OF_DEGREE[Gbl.Hierarchy.Crs.Year],
			  NewDeg.FullName,
		          Gbl.Hierarchy.Crs.FullName);
      else	// Update degree in database
	{
	 /***** Update degree in table of courses *****/
	 CrsCfg_UpdateCrsDegDB (Gbl.Hierarchy.Crs.CrsCod,NewDeg.DegCod);
	 Gbl.Hierarchy.Crs.DegCod =
	 Gbl.Hierarchy.Deg.DegCod = NewDeg.DegCod;

	 /***** Initialize again current course, degree, centre... *****/
      	 Hie_InitHierarchy ();

	 /***** Create alert to show the change made *****/
	 Ale_CreateAlert (Ale_SUCCESS,NULL,
	                  Txt_The_course_X_has_been_moved_to_the_degree_Y,
		          Gbl.Hierarchy.Crs.FullName,
		          Gbl.Hierarchy.Deg.FullName);
	}
     }
  }

/*****************************************************************************/
/********************** Update degree in table of courses ********************/
/*****************************************************************************/

static void CrsCfg_UpdateCrsDegDB (long CrsCod,long DegCod)
  {
   /***** Update degree in table of courses *****/
   DB_QueryUPDATE ("can not move course to another degree",
		   "UPDATE courses SET DegCod=%ld WHERE CrsCod=%ld",
	           DegCod,CrsCod);
  }

/*****************************************************************************/
/*************** Change the name of a course in configuration ****************/
/*****************************************************************************/

void CrsCfg_RenameCourseShort (void)
  {
   Crs_RenameCourse (&Gbl.Hierarchy.Crs,Cns_SHRT_NAME);
  }

void CrsCfg_RenameCourseFull (void)
  {
   Crs_RenameCourse (&Gbl.Hierarchy.Crs,Cns_FULL_NAME);
  }

/*****************************************************************************/
/*********** Change the year of a course in course configuration *************/
/*****************************************************************************/

void CrsCfg_ChangeCrsYear (void)
  {
   extern const char *Txt_The_course_X_already_exists_in_year_Y;
   extern const char *Txt_YEAR_OF_DEGREE[1 + Deg_MAX_YEARS_PER_DEGREE];
   extern const char *Txt_The_year_of_the_course_X_has_changed;
   extern const char *Txt_The_year_X_is_not_allowed;
   char YearStr[2 + 1];
   unsigned NewYear;

   /***** Get parameter with year/semester *****/
   Par_GetParToText ("OthCrsYear",YearStr,2);
   NewYear = Deg_ConvStrToYear (YearStr);

   if (NewYear <= Deg_MAX_YEARS_PER_DEGREE)	// If year is valid
     {
      /***** If name of course was in database in the new year... *****/
      if (Crs_CheckIfCrsNameExistsInYearOfDeg ("ShortName",Gbl.Hierarchy.Crs.ShrtName,-1L,
                                               Gbl.Hierarchy.Crs.DegCod,NewYear))
	 Ale_CreateAlert (Ale_WARNING,NULL,
	                  Txt_The_course_X_already_exists_in_year_Y,
		          Gbl.Hierarchy.Crs.ShrtName,
			  Txt_YEAR_OF_DEGREE[NewYear]);
      else if (Crs_CheckIfCrsNameExistsInYearOfDeg ("FullName",Gbl.Hierarchy.Crs.FullName,-1L,
                                                    Gbl.Hierarchy.Crs.DegCod,NewYear))
	 Ale_CreateAlert (Ale_WARNING,NULL,
	                  Txt_The_course_X_already_exists_in_year_Y,
		          Gbl.Hierarchy.Crs.FullName,
			  Txt_YEAR_OF_DEGREE[NewYear]);
      else	// Update year in database
	{
	 /***** Update year in table of courses *****/
         Crs_UpdateCrsYear (&Gbl.Hierarchy.Crs,NewYear);

	 /***** Create alert to show the change made *****/
	 Ale_CreateAlert (Ale_SUCCESS,NULL,
	                  Txt_The_year_of_the_course_X_has_changed,
		          Gbl.Hierarchy.Crs.ShrtName);
	}
     }
   else	// Year not valid
      Ale_CreateAlert (Ale_WARNING,NULL,
	               Txt_The_year_X_is_not_allowed,
		       NewYear);
  }

/*****************************************************************************/
/***** Change the institutional code of a course in course configuration *****/
/*****************************************************************************/

void CrsCfg_ChangeInsCrsCod (void)
  {
   extern const char *Txt_The_institutional_code_of_the_course_X_has_changed_to_Y;
   extern const char *Txt_The_institutional_code_of_the_course_X_has_not_changed;
   char NewInstitutionalCrsCod[Crs_MAX_BYTES_INSTITUTIONAL_CRS_COD + 1];

   /***** Get institutional code from form *****/
   Par_GetParToText ("InsCrsCod",NewInstitutionalCrsCod,Crs_MAX_BYTES_INSTITUTIONAL_CRS_COD);

   /***** Change the institutional course code *****/
   if (strcmp (NewInstitutionalCrsCod,Gbl.Hierarchy.Crs.InstitutionalCrsCod))
     {
      Crs_UpdateInstitutionalCrsCod (&Gbl.Hierarchy.Crs,NewInstitutionalCrsCod);

      Ale_CreateAlert (Ale_SUCCESS,NULL,
	               Txt_The_institutional_code_of_the_course_X_has_changed_to_Y,
	               Gbl.Hierarchy.Crs.ShrtName,
		       NewInstitutionalCrsCod);
     }
   else	// The same institutional code
      Ale_CreateAlert (Ale_INFO,NULL,
	               Txt_The_institutional_code_of_the_course_X_has_not_changed,
	               Gbl.Hierarchy.Crs.ShrtName);
  }

/*****************************************************************************/
/** Show message of success after changing a course in course configuration **/
/*****************************************************************************/

void CrsCfg_ContEditAfterChgCrs (void)
  {
   /***** Write error/success message *****/
   Ale_ShowAlerts (NULL);

   /***** Show the form again *****/
   Crs_ShowIntroduction ();
  }
