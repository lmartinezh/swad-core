// swad_hierarchy.c: hierarchy (system, institution, centre, degree, course)

/*
    SWAD (Shared Workspace At a Distance),
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

#include <stdio.h>		// For fprintf, etc.
#include <string.h>		// For string functions

#include "swad_config.h"
#include "swad_degree.h"
#include "swad_form.h"
#include "swad_global.h"
#include "swad_HTML.h"
#include "swad_logo.h"
#include "swad_theme.h"

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
/**************************** Private constants ******************************/
/*****************************************************************************/

/*****************************************************************************/
/**************************** Private prototypes *****************************/
/*****************************************************************************/

/*****************************************************************************/
/********** List pending institutions, centres, degrees and courses **********/
/*****************************************************************************/

void Hie_SeePending (void)
  {
   /***** Put contextual links *****/
   HTM_DIV_Begin ("class=\"CONTEXT_MENU\"");

   /* Put link to remove old courses */
   Crs_PutLinkToRemoveOldCrss ();

   HTM_DIV_End ();

   /***** List countries with pending institutions *****/
   Cty_SeeCtyWithPendingInss ();

   /***** List institutions with pending centres *****/
   Ins_SeeInsWithPendingCtrs ();

   /***** List centres with pending degrees *****/
   Ctr_SeeCtrWithPendingDegs ();

   /***** List degrees with pending courses *****/
   Deg_SeeDegWithPendingCrss ();
  }

/*****************************************************************************/
/*** Write menu to select country, institution, centre, degree and course ****/
/*****************************************************************************/

void Hie_WriteMenuHierarchy (void)
  {
   extern const char *The_ClassFormInBox[The_NUM_THEMES];
   extern const char *Txt_Country;
   extern const char *Txt_Institution;
   extern const char *Txt_Centre;
   extern const char *Txt_Degree;
   extern const char *Txt_Course;

   /***** Begin table *****/
   HTM_TABLE_BeginCenterPadding (2);

   /***** Write a 1st selector
          with all the countries *****/
   HTM_TR_Begin (NULL);

   HTM_TD_Begin ("class=\"RM\"");
   fprintf (Gbl.F.Out,"<label for=\"cty\" class=\"%s\">%s:</label>",
            The_ClassFormInBox[Gbl.Prefs.Theme],Txt_Country);
   HTM_TD_End ();

   HTM_TD_Begin ("class=\"LM\"");
   Cty_WriteSelectorOfCountry ();
   HTM_TD_End ();

   HTM_TR_End ();

   if (Gbl.Hierarchy.Cty.CtyCod > 0)
     {
      /***** Write a 2nd selector
             with the institutions of selected country *****/
      HTM_TR_Begin (NULL);

      HTM_TD_Begin ("class=\"RM\"");
      fprintf (Gbl.F.Out,"<label for=\"ins\" class=\"%s\">%s:</label>",
               The_ClassFormInBox[Gbl.Prefs.Theme],Txt_Institution);
      HTM_TD_End ();

      HTM_TD_Begin ("class=\"LM\"");
      Ins_WriteSelectorOfInstitution ();
      HTM_TD_End ();

      HTM_TR_End ();

      if (Gbl.Hierarchy.Ins.InsCod > 0)
        {
         /***** Write a 3rd selector
                with all the centres of selected institution *****/
         HTM_TR_Begin (NULL);

         HTM_TD_Begin ("class=\"RM\"");
         fprintf (Gbl.F.Out,"<label for=\"ctr\" class=\"%s\">%s:</label>",
                  The_ClassFormInBox[Gbl.Prefs.Theme],Txt_Centre);
         HTM_TD_End ();

         HTM_TD_Begin ("class=\"LM\"");
         Ctr_WriteSelectorOfCentre ();
         HTM_TD_End ();

         HTM_TR_End ();

         if (Gbl.Hierarchy.Ctr.CtrCod > 0)
           {
            /***** Write a 4th selector
                   with all the degrees of selected centre *****/
            HTM_TR_Begin (NULL);

	    HTM_TD_Begin ("class=\"RM\"");
	    fprintf (Gbl.F.Out,"<label for=\"deg\" class=\"%s\">%s:</label>",
                     The_ClassFormInBox[Gbl.Prefs.Theme],Txt_Degree);
	    HTM_TD_End ();

            HTM_TD_Begin ("class=\"LM\"");
            Deg_WriteSelectorOfDegree ();
            HTM_TD_End ();

            HTM_TR_End ();

	    if (Gbl.Hierarchy.Deg.DegCod > 0)
	      {
	       /***** Write a 5th selector
		      with all the courses of selected degree *****/
	       HTM_TR_Begin (NULL);

	       HTM_TD_Begin ("class=\"RM\"");
	       fprintf (Gbl.F.Out,"<label for=\"crs\" class=\"%s\">%s:</label>",
			The_ClassFormInBox[Gbl.Prefs.Theme],Txt_Course);
	       HTM_TD_End ();

	       HTM_TD_Begin ("class=\"LM\"");
	       Crs_WriteSelectorOfCourse ();
	       HTM_TD_End ();

	       HTM_TR_End ();
	      }
           }
        }
     }

   /***** End table *****/
   HTM_TABLE_End ();
  }

/*****************************************************************************/
/************* Write hierarchy breadcrumb in the top of the page *************/
/*****************************************************************************/

void Hie_WriteHierarchyInBreadcrumb (void)
  {
   extern const char *The_ClassBreadcrumb[The_NUM_THEMES];
   extern const char *Txt_System;
   extern const char *Txt_Country;
   extern const char *Txt_Institution;
   extern const char *Txt_Centre;
   extern const char *Txt_Degree;
   const char *ClassTxt = The_ClassBreadcrumb[Gbl.Prefs.Theme];

   /***** Form to go to the system *****/
   HTM_DIV_Begin ("class=\"BC %s\"",ClassTxt);
   fprintf (Gbl.F.Out,"&nbsp;");

   Frm_StartFormGoTo (ActMnu);
   Par_PutHiddenParamUnsigned ("NxtTab",(unsigned) TabSys);
   Frm_LinkFormSubmit (Txt_System,ClassTxt,NULL);
   fprintf (Gbl.F.Out,"%s</a>",Txt_System);
   Frm_EndForm ();

   HTM_DIV_End ();

   if (Gbl.Hierarchy.Cty.CtyCod > 0)		// Country selected...
     {
      HTM_DIV_Begin ("class=\"BC %s\"",ClassTxt);

      /***** Separator *****/
      fprintf (Gbl.F.Out,"&nbsp;&gt;&nbsp;");

      /***** Form to go to see institutions of this country *****/
      Frm_StartFormGoTo (ActSeeIns);
      Cty_PutParamCtyCod (Gbl.Hierarchy.Cty.CtyCod);
      Frm_LinkFormSubmit (Gbl.Hierarchy.Cty.Name[Gbl.Prefs.Language],ClassTxt,NULL);
      fprintf (Gbl.F.Out,"%s</a>",Gbl.Hierarchy.Cty.Name[Gbl.Prefs.Language]);
      Frm_EndForm ();

      HTM_DIV_End ();
     }
   else
     {
      HTM_DIV_Begin ("class=\"BC BC_SEMIOFF %s\"",ClassTxt);

      /***** Separator *****/
      fprintf (Gbl.F.Out,"&nbsp;&gt;&nbsp;");

      /***** Form to go to select countries *****/
      Frm_StartFormGoTo (ActSeeCty);
      Frm_LinkFormSubmit (Txt_Country,ClassTxt,NULL);
      fprintf (Gbl.F.Out,"%s</a>",Txt_Country);
      Frm_EndForm ();

      HTM_DIV_End ();
     }

   if (Gbl.Hierarchy.Ins.InsCod > 0)		// Institution selected...
     {
      HTM_DIV_Begin ("class=\"BC %s\"",ClassTxt);

      /***** Separator *****/
      fprintf (Gbl.F.Out,"&nbsp;&gt;&nbsp;");

      /***** Form to see centres of this institution *****/
      Frm_StartFormGoTo (ActSeeCtr);
      Ins_PutParamInsCod (Gbl.Hierarchy.Ins.InsCod);
      Frm_LinkFormSubmit (Gbl.Hierarchy.Ins.FullName,ClassTxt,NULL);
      fprintf (Gbl.F.Out,"%s</a>",Gbl.Hierarchy.Ins.ShrtName);
      Frm_EndForm ();

      HTM_DIV_End ();
     }
   else if (Gbl.Hierarchy.Cty.CtyCod > 0)
     {
      HTM_DIV_Begin ("class=\"BC BC_SEMIOFF %s\"",ClassTxt);

      /***** Separator *****/
      fprintf (Gbl.F.Out,"&nbsp;&gt;&nbsp;");

      /***** Form to go to select institutions *****/
      Frm_StartFormGoTo (ActSeeIns);
      Frm_LinkFormSubmit (Txt_Institution,ClassTxt,NULL);
      fprintf (Gbl.F.Out,"%s</a>",Txt_Institution);
      Frm_EndForm ();

      HTM_DIV_End ();
     }
   else
     {
      HTM_DIV_Begin ("class=\"BC BC_OFF %s\"",ClassTxt);

      /***** Separator *****/
      fprintf (Gbl.F.Out,"&nbsp;&gt;&nbsp;");

      /***** Hidden institution *****/
      fprintf (Gbl.F.Out,"%s",Txt_Institution);

      HTM_DIV_End ();
     }

   if (Gbl.Hierarchy.Ctr.CtrCod > 0)	// Centre selected...
     {
      HTM_DIV_Begin ("class=\"BC %s\"",ClassTxt);

      /***** Separator *****/
      fprintf (Gbl.F.Out,"&nbsp;&gt;&nbsp;");

      /***** Form to see degrees of this centre *****/
      Frm_StartFormGoTo (ActSeeDeg);
      Ctr_PutParamCtrCod (Gbl.Hierarchy.Ctr.CtrCod);
      Frm_LinkFormSubmit (Gbl.Hierarchy.Ctr.FullName,ClassTxt,NULL);
      fprintf (Gbl.F.Out,"%s</a>",Gbl.Hierarchy.Ctr.ShrtName);
      Frm_EndForm ();

      HTM_DIV_End ();
     }
   else if (Gbl.Hierarchy.Ins.InsCod > 0)
     {
      HTM_DIV_Begin ("class=\"BC BC_SEMIOFF %s\"",ClassTxt);

      /***** Separator *****/
      fprintf (Gbl.F.Out,"&nbsp;&gt;&nbsp;");

      /***** Form to go to select centres *****/
      Frm_StartFormGoTo (ActSeeCtr);
      Frm_LinkFormSubmit (Txt_Centre,ClassTxt,NULL);
      fprintf (Gbl.F.Out,"%s</a>",Txt_Centre);
      Frm_EndForm ();

      HTM_DIV_End ();
     }
   else
     {
      HTM_DIV_Begin ("class=\"BC BC_OFF %s\"",ClassTxt);

      /***** Separator *****/
      fprintf (Gbl.F.Out,"&nbsp;&gt;&nbsp;");

      /***** Hidden centre *****/
      fprintf (Gbl.F.Out,"%s",Txt_Centre);

      HTM_DIV_End ();
     }

   if (Gbl.Hierarchy.Deg.DegCod > 0)	// Degree selected...
     {
      HTM_DIV_Begin ("class=\"BC %s\"",ClassTxt);

      /***** Separator *****/
      fprintf (Gbl.F.Out,"&nbsp;&gt;&nbsp;");

      /***** Form to go to see courses of this degree *****/
      Frm_StartFormGoTo (ActSeeCrs);
      Deg_PutParamDegCod (Gbl.Hierarchy.Deg.DegCod);
      Frm_LinkFormSubmit (Gbl.Hierarchy.Deg.FullName,ClassTxt,NULL);
      fprintf (Gbl.F.Out,"%s</a>",Gbl.Hierarchy.Deg.ShrtName);
      Frm_EndForm ();

      HTM_DIV_End ();
     }
   else if (Gbl.Hierarchy.Ctr.CtrCod > 0)
     {
      HTM_DIV_Begin ("class=\"BC BC_SEMIOFF %s\"",ClassTxt);

      /***** Separator *****/
      fprintf (Gbl.F.Out,"&nbsp;&gt;&nbsp;");

      /***** Form to go to select degrees *****/
      Frm_StartFormGoTo (ActSeeDeg);
      Frm_LinkFormSubmit (Txt_Degree,ClassTxt,NULL);
      fprintf (Gbl.F.Out,"%s</a>",Txt_Degree);
      Frm_EndForm ();

      HTM_DIV_End ();
     }
   else
     {
      HTM_DIV_Begin ("class=\"BC BC_OFF %s\"",ClassTxt);

      /***** Separator *****/
      fprintf (Gbl.F.Out,"&nbsp;&gt;&nbsp;");

      /***** Hidden degree *****/
      fprintf (Gbl.F.Out,"%s",Txt_Degree);

      HTM_DIV_End ();
     }

   HTM_DIV_Begin ("class=\"BC%s %s\"",
		   (Gbl.Hierarchy.Level == Hie_CRS) ? "" :
		  ((Gbl.Hierarchy.Deg.DegCod > 0) ? " BC_SEMIOFF" :
						    " BC_OFF"),
		  ClassTxt);

   /***** Separator *****/
   fprintf (Gbl.F.Out,"&nbsp;&gt;&nbsp;");

   HTM_DIV_End ();
  }

/*****************************************************************************/
/*************** Write course full name in the top of the page ***************/
/*****************************************************************************/

void Hie_WriteBigNameCtyInsCtrDegCrs (void)
  {
   extern const char *The_ClassCourse[The_NUM_THEMES];
   extern const char *Txt_TAGLINE;

   fprintf (Gbl.F.Out,"<h1 id=\"main_title\" class=\"%s\">",
	    The_ClassCourse[Gbl.Prefs.Theme]);

   /***** Logo *****/
   switch (Gbl.Hierarchy.Level)
     {
      case Hie_SYS:	// System
	 fprintf (Gbl.F.Out,"<img src=\"%s/swad64x64.png\""
			    " alt=\"%s\" title=\"%s\""
			    " class=\"ICO40x40 TOP_LOGO\" />",
		  Cfg_URL_ICON_PUBLIC,
		  Cfg_PLATFORM_SHORT_NAME,Cfg_PLATFORM_FULL_NAME);
         break;
      case Hie_CTY:	// Country
         Cty_DrawCountryMap (&Gbl.Hierarchy.Cty,"COUNTRY_MAP_TITLE");
         break;
      case Hie_INS:	// Institution
	 Log_DrawLogo (Hie_INS,Gbl.Hierarchy.Ins.InsCod,
		       Gbl.Hierarchy.Ins.ShrtName,40,"TOP_LOGO",false);
         break;
      case Hie_CTR:	// Centre
	 Log_DrawLogo (Hie_CTR,Gbl.Hierarchy.Ctr.CtrCod,
		       Gbl.Hierarchy.Ctr.ShrtName,40,"TOP_LOGO",false);
         break;
      case Hie_DEG:	// Degree
      case Hie_CRS:	// Course
	 Log_DrawLogo (Hie_DEG,Gbl.Hierarchy.Deg.DegCod,
		       Gbl.Hierarchy.Deg.ShrtName,40,"TOP_LOGO",false);
         break;
      default:
	 break;
     }

   /***** Text *****/
   HTM_DIV_Begin ("id=\"big_name_container\"");
   if (Gbl.Hierarchy.Cty.CtyCod > 0)
     {
      HTM_DIV_Begin ("id=\"big_full_name\"");
      fprintf (Gbl.F.Out,"%s",		// Full name
		(Gbl.Hierarchy.Level == Hie_CRS) ? Gbl.Hierarchy.Crs.FullName :
	       ((Gbl.Hierarchy.Level == Hie_DEG) ? Gbl.Hierarchy.Deg.FullName :
	       ((Gbl.Hierarchy.Level == Hie_CTR) ? Gbl.Hierarchy.Ctr.FullName :
	       ((Gbl.Hierarchy.Level == Hie_INS) ? Gbl.Hierarchy.Ins.FullName :
	                                           Gbl.Hierarchy.Cty.Name[Gbl.Prefs.Language]))));
      HTM_DIV_End ();

      HTM_DIV_Begin ("class=\"NOT_SHOWN\"");
      fprintf (Gbl.F.Out," / ");	// To separate
      HTM_DIV_End ();

      HTM_DIV_Begin ("id=\"big_short_name\"");
      fprintf (Gbl.F.Out,"%s",		// Short name
		(Gbl.Hierarchy.Level == Hie_CRS) ? Gbl.Hierarchy.Crs.ShrtName :
	       ((Gbl.Hierarchy.Level == Hie_DEG) ? Gbl.Hierarchy.Deg.ShrtName :
	       ((Gbl.Hierarchy.Level == Hie_CTR) ? Gbl.Hierarchy.Ctr.ShrtName :
	       ((Gbl.Hierarchy.Level == Hie_INS) ? Gbl.Hierarchy.Ins.ShrtName :
	                                           Gbl.Hierarchy.Cty.Name[Gbl.Prefs.Language]))));
      HTM_DIV_End ();
     }
   else	// No country specified ==> home page
     {
      HTM_DIV_Begin ("id=\"big_full_name\"");
      fprintf (Gbl.F.Out,"%s: %s",	// Full name
	       Cfg_PLATFORM_SHORT_NAME,Txt_TAGLINE);
      HTM_DIV_End ();

      HTM_DIV_Begin ("class=\"NOT_SHOWN\"");
      fprintf (Gbl.F.Out," / ");	// To separate
      HTM_DIV_End ();

      HTM_DIV_Begin ("id=\"big_short_name\"");
      fprintf (Gbl.F.Out,"%s",		// Short name
	       Cfg_PLATFORM_SHORT_NAME);
      HTM_DIV_End ();
     }
   HTM_DIV_End ();
   fprintf (Gbl.F.Out,"</h1>");
  }

/*****************************************************************************/
/**************** Copy last hierarchy to current hierarchy *******************/
/*****************************************************************************/

void Hie_SetHierarchyFromUsrLastHierarchy (void)
  {
   /***** Initialize all codes to -1 *****/
   Hie_ResetHierarchy ();

   /***** Copy last hierarchy scope and code to current hierarchy *****/
   switch (Gbl.Usrs.Me.UsrLast.LastHie.Scope)
     {
      case Hie_CTY:	// Country
         Gbl.Hierarchy.Cty.CtyCod = Gbl.Usrs.Me.UsrLast.LastHie.Cod;
	 break;
      case Hie_INS:	// Institution
         Gbl.Hierarchy.Ins.InsCod = Gbl.Usrs.Me.UsrLast.LastHie.Cod;
	 break;
      case Hie_CTR:	// Centre
         Gbl.Hierarchy.Ctr.CtrCod = Gbl.Usrs.Me.UsrLast.LastHie.Cod;
	 break;
      case Hie_DEG:	// Degree
         Gbl.Hierarchy.Deg.DegCod = Gbl.Usrs.Me.UsrLast.LastHie.Cod;
	 break;
      case Hie_CRS:	// Course
         Gbl.Hierarchy.Crs.CrsCod = Gbl.Usrs.Me.UsrLast.LastHie.Cod;
	 break;
      default:
	 break;
     }

   /****** Initialize again current course, degree, centre... ******/
   Hie_InitHierarchy ();
  }

/*****************************************************************************/
/**** Initialize current country, institution, centre, degree and course *****/
/*****************************************************************************/

void Hie_InitHierarchy (void)
  {
   /***** If course code is available, get course data *****/
   if (Gbl.Hierarchy.Crs.CrsCod > 0)
     {
      if (Crs_GetDataOfCourseByCod (&Gbl.Hierarchy.Crs))				// Course found
         Gbl.Hierarchy.Deg.DegCod = Gbl.Hierarchy.Crs.DegCod;
      else
         Hie_ResetHierarchy ();
     }

   /***** If degree code is available, get degree data *****/
   if (Gbl.Hierarchy.Deg.DegCod > 0)
     {
      if (Deg_GetDataOfDegreeByCod (&Gbl.Hierarchy.Deg))				// Degree found
	{
	 Gbl.Hierarchy.Ctr.CtrCod = Gbl.Hierarchy.Deg.CtrCod;
         Gbl.Hierarchy.Ins.InsCod = Deg_GetInsCodOfDegreeByCod (Gbl.Hierarchy.Deg.DegCod);
	}
      else
         Hie_ResetHierarchy ();
     }

   /***** If centre code is available, get centre data *****/
   if (Gbl.Hierarchy.Ctr.CtrCod > 0)
     {
      if (Ctr_GetDataOfCentreByCod (&Gbl.Hierarchy.Ctr))				// Centre found
         Gbl.Hierarchy.Ins.InsCod = Gbl.Hierarchy.Ctr.InsCod;
      else
         Hie_ResetHierarchy ();
     }

   /***** If institution code is available, get institution data *****/
   if (Gbl.Hierarchy.Ins.InsCod > 0)
     {
      if (Ins_GetDataOfInstitutionByCod (&Gbl.Hierarchy.Ins,Ins_GET_BASIC_DATA))	// Institution found
	 Gbl.Hierarchy.Cty.CtyCod = Gbl.Hierarchy.Ins.CtyCod;
      else
         Hie_ResetHierarchy ();
     }

   /***** If country code is available, get country data *****/
   if (Gbl.Hierarchy.Cty.CtyCod > 0)
     {
      if (!Cty_GetDataOfCountryByCod (&Gbl.Hierarchy.Cty,Cty_GET_BASIC_DATA))		// Country not found
         Hie_ResetHierarchy ();
     }

   /***** Set current hierarchy level and code
          depending on course code, degree code, etc. *****/
   if      (Gbl.Hierarchy.Crs.CrsCod > 0)	// Course selected
     {
      Gbl.Hierarchy.Level = Hie_CRS;
      Gbl.Hierarchy.Cod = Gbl.Hierarchy.Crs.CrsCod;
     }
   else if (Gbl.Hierarchy.Deg.DegCod > 0)	// Degree selected
     {
      Gbl.Hierarchy.Level = Hie_DEG;
      Gbl.Hierarchy.Cod = Gbl.Hierarchy.Deg.DegCod;
     }
   else if (Gbl.Hierarchy.Ctr.CtrCod > 0)	// Centre selected
     {
      Gbl.Hierarchy.Level = Hie_CTR;
      Gbl.Hierarchy.Cod = Gbl.Hierarchy.Ctr.CtrCod;
     }
   else if (Gbl.Hierarchy.Ins.InsCod > 0)	// Institution selected
     {
      Gbl.Hierarchy.Level = Hie_INS;
      Gbl.Hierarchy.Cod = Gbl.Hierarchy.Ins.InsCod;
     }
   else if (Gbl.Hierarchy.Cty.CtyCod > 0)	// Country selected
     {
      Gbl.Hierarchy.Level = Hie_CTY;
      Gbl.Hierarchy.Cod = Gbl.Hierarchy.Cty.CtyCod;
     }
   else
     {
      Gbl.Hierarchy.Level = Hie_SYS;
      Gbl.Hierarchy.Cod = -1L;
     }

   /***** Initialize paths *****/
   if (Gbl.Hierarchy.Level == Hie_CRS)	// Course selected
     {
      /***** Paths of course directories *****/
      snprintf (Gbl.Crs.PathPriv,sizeof (Gbl.Crs.PathPriv),
	        "%s/%ld",
	        Cfg_PATH_CRS_PRIVATE,Gbl.Hierarchy.Crs.CrsCod);
      snprintf (Gbl.Crs.PathRelPubl,sizeof (Gbl.Crs.PathRelPubl),
	        "%s/%ld",
	        Cfg_PATH_CRS_PUBLIC,Gbl.Hierarchy.Crs.CrsCod);
      snprintf (Gbl.Crs.PathURLPubl,sizeof (Gbl.Crs.PathURLPubl),
	        "%s/%ld",
	        Cfg_URL_CRS_PUBLIC,Gbl.Hierarchy.Crs.CrsCod);

      /***** If any of the course directories does not exist, create it *****/
      if (!Fil_CheckIfPathExists (Gbl.Crs.PathPriv))
	 Fil_CreateDirIfNotExists (Gbl.Crs.PathPriv);
      if (!Fil_CheckIfPathExists (Gbl.Crs.PathRelPubl))
	 Fil_CreateDirIfNotExists (Gbl.Crs.PathRelPubl);

      /***** Count number of groups in current course
             (used in some actions) *****/
      Gbl.Crs.Grps.NumGrps = Grp_CountNumGrpsInCurrentCrs ();
     }
  }

/*****************************************************************************/
/******* Reset current country, institution, centre, degree and course *******/
/*****************************************************************************/

void Hie_ResetHierarchy (void)
  {
   /***** Country *****/
   Gbl.Hierarchy.Cty.CtyCod = -1L;

   /***** Institution *****/
   Gbl.Hierarchy.Ins.InsCod = -1L;

   /***** Centre *****/
   Gbl.Hierarchy.Ctr.CtrCod = -1L;
   Gbl.Hierarchy.Ctr.InsCod = -1L;
   Gbl.Hierarchy.Ctr.PlcCod = -1L;

   /***** Degree *****/
   Gbl.Hierarchy.Deg.DegCod = -1L;

   /***** Course *****/
   Gbl.Hierarchy.Crs.CrsCod = -1L;

   /***** Hierarchy level and code *****/
   Gbl.Hierarchy.Level = Hie_UNK;
   Gbl.Hierarchy.Cod   = -1L;
  }
