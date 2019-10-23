// swad_tab.c: tabs drawing

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

#include "swad_form.h"
#include "swad_global.h"
#include "swad_HTML.h"
#include "swad_parameter.h"
#include "swad_tab.h"

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/****************************** Private constants ****************************/
/*****************************************************************************/

const char *Tab_TabIcons[Tab_NUM_TABS] =
  {
   /* TabUnk */	NULL,
   /* TabStr */ "home",
   /* TabSys */	"sitemap",
   /* TabCty */	"globe",
   /* TabIns */	"university",
   /* TabCtr */	"building",
   /* TabDeg */	"graduation-cap",
   /* TabCrs */	"list-ol",
   /* TabAss */	"check",
   /* TabFil */	"folder-open",
   /* TabUsr */	"users",
   /* TabMsg */	"envelope",
   /* TabAna */	"chart-bar",
   /* TabPrf */	"user",
  };

/*****************************************************************************/
/******************************* Private types *******************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Private variables *****************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Private prototypes ****************************/
/*****************************************************************************/

static bool Tab_CheckIfICanViewTab (Tab_Tab_t Tab);
static const char *Tab_GetIcon (Tab_Tab_t Tab);

/*****************************************************************************/
/**************** Draw tabs with the current tab highlighted *****************/
/*****************************************************************************/

void Tab_DrawTabs (void)
  {
   extern const char *The_ClassTxtTabOn[The_NUM_THEMES];
   extern const char *The_ClassTxtTabOff[The_NUM_THEMES];
   extern const char *The_TabOnBgColors[The_NUM_THEMES];
   extern const char *The_TabOffBgColors[The_NUM_THEMES];
   extern const char *Txt_TABS_TXT[Tab_NUM_TABS];
   extern const char *Txt_TABS_TXT[Tab_NUM_TABS];
   Tab_Tab_t NumTab;
   bool ICanViewTab;
   const char *ClassHeadRow3[The_NUM_THEMES] =
     {
      "HEAD_ROW_3_WHITE",	// The_THEME_WHITE
      "HEAD_ROW_3_GREY",	// The_THEME_GREY
      "HEAD_ROW_3_PURPLE",	// The_THEME_PURPLE
      "HEAD_ROW_3_BLUE",	// The_THEME_BLUE
      "HEAD_ROW_3_YELLOW",	// The_THEME_YELLOW
      "HEAD_ROW_3_PINK",	// The_THEME_PINK
      };

   /***** Start tabs container *****/
   HTM_DIV_Begin ("class=\"%s\"",ClassHeadRow3[Gbl.Prefs.Theme]);
   fprintf (Gbl.F.Out,"<nav id=\"tabs\">"
	              "<ul class=\"LIST_TABS\">");

   /***** Draw the tabs *****/
   for (NumTab = (Tab_Tab_t) 1;
        NumTab <= (Tab_Tab_t) Tab_NUM_TABS - 1;
        NumTab++)
     {
      ICanViewTab = Tab_CheckIfICanViewTab (NumTab);

      /* If current tab is unknown, then activate the first one with access allowed */
      if (Gbl.Action.Tab == TabUnk)
	{
	 Gbl.Action.Tab = NumTab;
	 Tab_DisableIncompatibleTabs ();
	}

      if (ICanViewTab)	// Don't show the first hidden tabs
	{
	 /* Form, icon (at top) and text (at bottom) of the tab */
	 fprintf (Gbl.F.Out,"<li class=\"%s %s\">",
		  NumTab == Gbl.Action.Tab ? "TAB_ON" :
					     "TAB_OFF",
		  NumTab == Gbl.Action.Tab ? The_TabOnBgColors[Gbl.Prefs.Theme] :
					     The_TabOffBgColors[Gbl.Prefs.Theme]);

	 fprintf (Gbl.F.Out,"<div");	// This div must be present even in current tab in order to render properly the tab
	 if (NumTab != Gbl.Action.Tab)
	    fprintf (Gbl.F.Out," class=\"ICO_HIGHLIGHT\"");
	 fprintf (Gbl.F.Out,">");

	 Frm_StartForm (ActMnu);
	 Par_PutHiddenParamUnsigned ("NxtTab",(unsigned) NumTab);
	 Frm_LinkFormSubmit (Txt_TABS_TXT[NumTab],
			     NumTab == Gbl.Action.Tab ? The_ClassTxtTabOn[Gbl.Prefs.Theme] :
							The_ClassTxtTabOff[Gbl.Prefs.Theme],NULL);
	 fprintf (Gbl.F.Out,"<img src=\"%s/%s\" alt=\"%s\" title=\"%s\""
			    " class=\"TAB_ICO\" />",
		  Gbl.Prefs.URLIconSet,
		  Tab_GetIcon (NumTab),
		  Txt_TABS_TXT[NumTab],
		  Txt_TABS_TXT[NumTab]);
	 HTM_DIV_Begin ("class=\"TAB_TXT %s\"",
			NumTab == Gbl.Action.Tab ? The_ClassTxtTabOn[Gbl.Prefs.Theme] :
						   The_ClassTxtTabOff[Gbl.Prefs.Theme]);
	 fprintf (Gbl.F.Out,"%s",Txt_TABS_TXT[NumTab]);
	 HTM_DIV_End ();
	 fprintf (Gbl.F.Out,"</a>");
	 Frm_EndForm ();

	 HTM_DIV_End ();
	 fprintf (Gbl.F.Out,"</li>");
	}
     }

   /***** End tabs container *****/
   fprintf (Gbl.F.Out,"</ul>"
	              "</nav>");
   HTM_DIV_End ();
  }

/*****************************************************************************/
/************************* Check if I can view a tab *************************/
/*****************************************************************************/

static bool Tab_CheckIfICanViewTab (Tab_Tab_t Tab)
  {
   switch (Tab)
     {
      case TabUnk:
	 return false;
      case TabSys:
	 return (Gbl.Hierarchy.Cty.CtyCod <= 0);	// No country selected
      case TabCty:
	 return (Gbl.Hierarchy.Cty.CtyCod > 0 &&	// Country selected
	         Gbl.Hierarchy.Ins.InsCod <= 0);	// No institution selected
      case TabIns:
	 return (Gbl.Hierarchy.Ins.InsCod > 0 &&	// Institution selected
	         Gbl.Hierarchy.Ctr.CtrCod <= 0);	// No centre selected
      case TabCtr:
	 return (Gbl.Hierarchy.Ctr.CtrCod > 0 &&	// Centre selected
	         Gbl.Hierarchy.Deg.DegCod <= 0);	// No degree selected
      case TabDeg:
	 return (Gbl.Hierarchy.Deg.DegCod > 0 &&	// Degree selected
	         Gbl.Hierarchy.Crs.CrsCod <= 0);	// No course selected
      case TabCrs:
	 return (Gbl.Hierarchy.Level == Hie_CRS);	// Course selected
      case TabAss:
	 return (Gbl.Hierarchy.Level == Hie_CRS ||	// Course selected
	         Gbl.Usrs.Me.Role.Logged >= Rol_STD);	// I belong to course or I am an admin
      case TabFil:
      	 return (Gbl.Hierarchy.Ins.InsCod > 0 ||	// Institution selected
	         Gbl.Usrs.Me.Logged);			// I'm logged
      default:
	 return true;
     }
  }

/*****************************************************************************/
/********************** Get icon associated to an action *********************/
/*****************************************************************************/

static const char *Tab_GetIcon (Tab_Tab_t NumTab)
  {
   if (NumTab < (Tab_Tab_t) 1 || NumTab >= Tab_NUM_TABS)
      return NULL;

   return Ico_GetIcon (Tab_TabIcons[NumTab]);
  }

/*****************************************************************************/
/*************** Set current tab depending on current action *****************/
/*****************************************************************************/

void Tab_SetCurrentTab (void)
  {
   Gbl.Action.Tab = Act_GetTab (Gbl.Action.Act);

   /***** Change action and tab if country, institution, centre or degree
          are incompatible with the current tab *****/
   switch (Gbl.Action.Tab)
     {
      case TabCty:
	 if (Gbl.Hierarchy.Cty.CtyCod <= 0)		// No country selected
	    Gbl.Action.Act = ActSeeCty;
	 break;
      case TabIns:
	 if (Gbl.Hierarchy.Ins.InsCod <= 0)		// No institution selected
	   {
	    if (Gbl.Hierarchy.Cty.CtyCod > 0)		// Country selected, but no institution selected
	       Gbl.Action.Act = ActSeeIns;
	    else					// No country selected
	       Gbl.Action.Act = ActSeeCty;
	  }
	break;
      case TabCtr:
	 if (Gbl.Hierarchy.Ctr.CtrCod <= 0)		// No centre selected
	   {
	    if (Gbl.Hierarchy.Ins.InsCod > 0)		// Institution selected, but no centre selected
	       Gbl.Action.Act = ActSeeCtr;
	    else if (Gbl.Hierarchy.Cty.CtyCod > 0)	// Country selected, but no institution selected
	       Gbl.Action.Act = ActSeeIns;
	    else					// No country selected
	       Gbl.Action.Act = ActSeeCty;
	   }
         break;
      case TabDeg:
         if (Gbl.Hierarchy.Deg.DegCod <= 0)		// No degree selected
	   {
	    if (Gbl.Hierarchy.Ctr.CtrCod > 0)		// Centre selected, but no degree selected
	       Gbl.Action.Act = ActSeeDeg;
	    else if (Gbl.Hierarchy.Ins.InsCod > 0)	// Institution selected, but no centre selected
	       Gbl.Action.Act = ActSeeCtr;
	    else if (Gbl.Hierarchy.Cty.CtyCod > 0)	// Country selected, but no institution selected
	       Gbl.Action.Act = ActSeeIns;
	    else					// No country selected
	       Gbl.Action.Act = ActSeeCty;
	   }
         break;
      default:
         break;
     }

   Gbl.Action.Tab = Act_GetTab (Act_GetSuperAction (Gbl.Action.Act));

   Tab_DisableIncompatibleTabs ();
  }

/*****************************************************************************/
/************************** Disable incompatible tabs ************************/
/*****************************************************************************/

void Tab_DisableIncompatibleTabs (void)
  {
   /***** Set country, institution, centre, degree and course depending on the current tab.
          This will disable tabs incompatible with the current one. *****/
   switch (Gbl.Action.Tab)
     {
      case TabSys:
	 Gbl.Hierarchy.Cty.CtyCod = -1L;
	 /* falls through */
	 /* no break */
      case TabCty:
	 Gbl.Hierarchy.Ins.InsCod = -1L;
	 /* falls through */
	 /* no break */
      case TabIns:
	 Gbl.Hierarchy.Ctr.CtrCod = -1L;
	 /* falls through */
	 /* no break */
      case TabCtr:
	 Gbl.Hierarchy.Deg.DegCod = -1L;
	 /* falls through */
	 /* no break */
      case TabDeg:
	 Gbl.Hierarchy.Crs.CrsCod = -1L;
	 break;
      default:
         break;
     }
  }
