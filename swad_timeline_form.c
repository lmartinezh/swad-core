// swad_timeline_form.c: social timeline forms

/*
    SWAD (Shared Workspace At a Distance),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2021 Antonio Ca�as Vargas

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General 3 License as
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
/*********************************** Headers *********************************/
/*****************************************************************************/

#define _GNU_SOURCE 		// For asprintf
#include <stdio.h>		// For asprintf

#include "swad_action.h"
#include "swad_global.h"
#include "swad_timeline.h"
#include "swad_timeline_form.h"

/*****************************************************************************/
/****************************** Public constants *****************************/
/*****************************************************************************/

/*****************************************************************************/
/************************* Private constants and types ***********************/
/*****************************************************************************/

#define TL_Frm_ICON_ELLIPSIS	"ellipsis-h.svg"

const Act_Action_t TL_Frm_ActionGbl[TL_Frm_NUM_ACTIONS] =
  {
   [TL_Frm_RECEIVE_POST] = ActRcvTL_PstGbl,
   [TL_Frm_RECEIVE_COMM] = ActRcvTL_ComGbl,
   [TL_Frm_REQ_REM_NOTE] = ActReqRemTL_PubGbl,
   [TL_Frm_REQ_REM_COMM] = ActReqRemTL_ComGbl,
   [TL_Frm_SHA_NOTE    ] = ActShaTL_NotGbl,
   [TL_Frm_UNS_NOTE    ] = ActUnsTL_NotGbl,
   [TL_Frm_FAV_NOTE    ] = ActFavTL_NotGbl,
   [TL_Frm_FAV_COMM    ] = ActFavTL_ComGbl,
   [TL_Frm_UNF_NOTE    ] = ActUnfTL_NotGbl,
   [TL_Frm_UNF_COMM    ] = ActUnfTL_ComGbl,
   [TL_Frm_ALL_SHA_NOTE] = ActAllShaTL_NotGbl,
   [TL_Frm_ALL_FAV_NOTE] = ActAllFavTL_NotGbl,
   [TL_Frm_ALL_FAV_COMM] = ActAllFavTL_ComGbl,
   [TL_Frm_SHO_HID_COMM] = ActShoHidTL_ComGbl,
  };
const Act_Action_t TL_Frm_ActionUsr[TL_Frm_NUM_ACTIONS] =
  {
   [TL_Frm_RECEIVE_POST] = ActRcvTL_PstUsr,
   [TL_Frm_RECEIVE_COMM] = ActRcvTL_ComUsr,
   [TL_Frm_REQ_REM_NOTE] = ActReqRemTL_PubUsr,
   [TL_Frm_REQ_REM_COMM] = ActReqRemTL_ComUsr,
   [TL_Frm_SHA_NOTE    ] = ActShaTL_NotUsr,
   [TL_Frm_UNS_NOTE    ] = ActUnsTL_NotUsr,
   [TL_Frm_FAV_NOTE    ] = ActFavTL_NotUsr,
   [TL_Frm_FAV_COMM    ] = ActFavTL_ComUsr,
   [TL_Frm_UNF_NOTE    ] = ActUnfTL_NotUsr,
   [TL_Frm_UNF_COMM    ] = ActUnfTL_ComUsr,
   [TL_Frm_ALL_SHA_NOTE] = ActAllShaTL_NotUsr,
   [TL_Frm_ALL_FAV_NOTE] = ActAllFavTL_NotUsr,
   [TL_Frm_ALL_FAV_COMM] = ActAllFavTL_ComUsr,
   [TL_Frm_SHO_HID_COMM] = ActShoHidTL_ComUsr,
  };

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/************************* Private global variables **************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Private prototypes ****************************/
/*****************************************************************************/

/*****************************************************************************/
/***************** Start a form in global or user timeline *******************/
/*****************************************************************************/

void TL_Frm_FormStart (const struct TL_Timeline *Timeline,TL_Frm_Action_t Action)
  {
   if (Gbl.Usrs.Other.UsrDat.UsrCod > 0)
     {
      /***** Start form in user timeline *****/
      Frm_StartFormAnchor (TL_Frm_ActionUsr[Action],"timeline");
      Usr_PutParamOtherUsrCodEncrypted (Gbl.Usrs.Other.UsrDat.EnUsrCod);
     }
   else
     {
      /***** Start form in global timeline *****/
      Frm_StartForm (TL_Frm_ActionGbl[Action]);
      Usr_PutHiddenParamWho (Timeline->Who);
     }
  }

/*****************************************************************************/
/********************* Form to show all favers/sharers ***********************/
/*****************************************************************************/

void TL_Frm_PutFormToSeeAllFaversSharers (TL_Frm_Action_t Action,
		                          const char *ParamFormat,long ParamCod,
                                          TL_Usr_HowManyUsrs_t HowManyUsrs)
  {
   extern const char *Txt_View_all_USERS;

   switch (HowManyUsrs)
     {
      case TL_Usr_SHOW_FEW_USRS:
	 /***** Form and icon to mark note as favourite *****/
	 TL_Frm_FormFavSha (Action,ParamFormat,ParamCod,
			    TL_Frm_ICON_ELLIPSIS,Txt_View_all_USERS);
	 break;
      case TL_Usr_SHOW_ALL_USRS:
         Ico_PutIconOff (TL_Frm_ICON_ELLIPSIS,Txt_View_all_USERS);
	 break;
     }
  }

/*****************************************************************************/
/******* Form to fav/unfav or share/unshare in global or user timeline *******/
/*****************************************************************************/

void TL_Frm_FormFavSha (TL_Frm_Action_t Action,
		        const char *ParamFormat,long ParamCod,
		        const char *Icon,const char *Title)
  {
   char *OnSubmit;
   char ParamStr[7 + Cns_MAX_DECIMAL_DIGITS_LONG + 1];

   /***** Create parameter string *****/
   sprintf (ParamStr,ParamFormat,ParamCod);

   /*
   +---------------------------------------------------------------------------+
   | div which content will be updated (parent of parent of form)              |
   | +---------------------+ +-------+ +-------------------------------------+ |
   | | div (parent of form)| | div   | | div for users                       | |
   | | +-----------------+ | | for   | | +------+ +------+ +------+ +------+ | |
   | | |    this form    | | | num.  | | |      | |      | |      | | form | | |
   | | | +-------------+ | | | of    | | | user | | user | | user | |  to  | | |
   | | | |   fav icon  | | | | users | | |   1  | |   2  | |   3  | | show | | |
   | | | +-------------+ | | |       | | |      | |      | |      | |  all | | |
   | | +-----------------+ | |       | | +------+ +------+ +------+ +------+ | |
   | +---------------------+ +-------+ +-------------------------------------+ |
   +---------------------------------------------------------------------------+
   */

   /***** Form and icon to mark note as favourite *****/
   /* Form with icon */
   if (Gbl.Usrs.Other.UsrDat.UsrCod > 0)
     {
      if (asprintf (&OnSubmit,"updateDivFaversSharers(this,"
			      "'act=%ld&ses=%s&%s&OtherUsrCod=%s');"
			      " return false;",	// return false is necessary to not submit form
		    Act_GetActCod (TL_Frm_ActionUsr[Action]),
		    Gbl.Session.Id,
		    ParamStr,
		    Gbl.Usrs.Other.UsrDat.EnUsrCod) < 0)
	 Lay_NotEnoughMemoryExit ();
      Frm_StartFormUniqueAnchorOnSubmit (ActUnk,"timeline",OnSubmit);
     }
   else
     {
      if (asprintf (&OnSubmit,"updateDivFaversSharers(this,"
			      "'act=%ld&ses=%s&%s');"
			      " return false;",	// return false is necessary to not submit form
		    Act_GetActCod (TL_Frm_ActionGbl[Action]),
		    Gbl.Session.Id,
		    ParamStr) < 0)
	 Lay_NotEnoughMemoryExit ();
      Frm_StartFormUniqueAnchorOnSubmit (ActUnk,NULL,OnSubmit);
     }
   Ico_PutIconLink (Icon,Title);
   Frm_EndForm ();

   /* Free allocated memory */
   free (OnSubmit);
  }

/*****************************************************************************/
/********** Form to show hidden coments in global or user timeline ***********/
/*****************************************************************************/

void TL_Frm_FormToShowHiddenComments (long NotCod,
				      char IdComments[Frm_MAX_BYTES_ID + 1],
				      unsigned NumInitialComments)
  {
   extern const char *The_ClassFormLinkInBox[The_NUM_THEMES];
   extern const char *Txt_See_the_previous_X_COMMENTS;
   char *OnSubmit;

   HTM_DIV_Begin ("id=\"exp_%s\" class=\"TL_EXPAND_COM TL_RIGHT_WIDTH\"",
		  IdComments);

   /***** Form and icon-text to show hidden comments *****/
   /* Begin form */
   if (Gbl.Usrs.Other.UsrDat.UsrCod > 0)
     {
      if (asprintf (&OnSubmit,"toggleComments('%s');"
	                      "updateDivHiddenComments(this,"
			      "'act=%ld&ses=%s&NotCod=%ld&IdComments=%s&NumHidCom=%u&OtherUsrCod=%s');"
			      " return false;",	// return false is necessary to not submit form
		    IdComments,
		    Act_GetActCod (TL_Frm_ActionUsr[TL_Frm_SHO_HID_COMM]),
		    Gbl.Session.Id,
		    NotCod,
		    IdComments,
		    NumInitialComments,
		    Gbl.Usrs.Other.UsrDat.EnUsrCod) < 0)
	 Lay_NotEnoughMemoryExit ();
      Frm_StartFormUniqueAnchorOnSubmit (ActUnk,"timeline",OnSubmit);
     }
   else
     {
      if (asprintf (&OnSubmit,"toggleComments('%s');"
	                      "updateDivHiddenComments(this,"
			      "'act=%ld&ses=%s&NotCod=%ld&IdComments=%s&NumHidCom=%u');"
			      " return false;",	// return false is necessary to not submit form
		    IdComments,
		    Act_GetActCod (TL_Frm_ActionGbl[TL_Frm_SHO_HID_COMM]),
		    Gbl.Session.Id,
		    NotCod,
		    IdComments,
		    NumInitialComments) < 0)
	 Lay_NotEnoughMemoryExit ();
      Frm_StartFormUniqueAnchorOnSubmit (ActUnk,NULL,OnSubmit);
     }

   /* Put icon and text with link to show the first hidden comments */
   HTM_BUTTON_SUBMIT_Begin (NULL,The_ClassFormLinkInBox[Gbl.Prefs.Theme],NULL);
   Ico_PutIconTextLink ("angle-up.svg",
			Str_BuildStringLong (Txt_See_the_previous_X_COMMENTS,
					     (long) NumInitialComments));
   Str_FreeString ();
   HTM_BUTTON_End ();

   /* End form */
   Frm_EndForm ();

   /* Free allocated memory */
   free (OnSubmit);

   HTM_DIV_End ();
  }
