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
#include "swad_timeline_comment.h"
#include "swad_timeline_form.h"
#include "swad_timeline_note.h"

/*****************************************************************************/
/****************************** Public constants *****************************/
/*****************************************************************************/

/*****************************************************************************/
/************************* Private constants and types ***********************/
/*****************************************************************************/

#define TL_Frm_ICON_ELLIPSIS	"ellipsis-h.svg"

const Act_Action_t TL_Frm_ActionGbl[TL_Frm_NUM_ACTIONS] =
  {
   [TL_Frm_RECEIVE_POST] = ActRcvPstGblTL,
   [TL_Frm_RECEIVE_COMM] = ActRcvComGblTL,
   [TL_Frm_REQ_REM_NOTE] = ActReqRemPubGblTL,
   [TL_Frm_REQ_REM_COMM] = ActReqRemComGblTL,
   [TL_Frm_REM_NOTE    ] = ActRemPubGblTL,
   [TL_Frm_REM_COMM    ] = ActRemComGblTL,
   [TL_Frm_SHA_NOTE    ] = ActShaNotGblTL,
   [TL_Frm_UNS_NOTE    ] = ActUnsNotGblTL,
   [TL_Frm_FAV_NOTE    ] = ActFavNotGblTL,
   [TL_Frm_FAV_COMM    ] = ActFavComGblTL,
   [TL_Frm_UNF_NOTE    ] = ActUnfNotGblTL,
   [TL_Frm_UNF_COMM    ] = ActUnfComGblTL,
   [TL_Frm_ALL_SHA_NOTE] = ActAllShaNotGblTL,
   [TL_Frm_ALL_FAV_NOTE] = ActAllFavNotGblTL,
   [TL_Frm_ALL_FAV_COMM] = ActAllFavComGblTL,
   [TL_Frm_SHO_HID_COMM] = ActShoHidComGblTL,
  };
const Act_Action_t TL_Frm_ActionUsr[TL_Frm_NUM_ACTIONS] =
  {
   [TL_Frm_RECEIVE_POST] = ActRcvPstUsrTL,
   [TL_Frm_RECEIVE_COMM] = ActRcvComUsrTL,
   [TL_Frm_REQ_REM_NOTE] = ActReqRemPubUsrTL,
   [TL_Frm_REQ_REM_COMM] = ActReqRemComUsrTL,
   [TL_Frm_REM_NOTE    ] = ActRemPubUsrTL,
   [TL_Frm_REM_COMM    ] = ActRemComUsrTL,
   [TL_Frm_SHA_NOTE    ] = ActShaNotUsrTL,
   [TL_Frm_UNS_NOTE    ] = ActUnsNotUsrTL,
   [TL_Frm_FAV_NOTE    ] = ActFavNotUsrTL,
   [TL_Frm_FAV_COMM    ] = ActFavComUsrTL,
   [TL_Frm_UNF_NOTE    ] = ActUnfNotUsrTL,
   [TL_Frm_UNF_COMM    ] = ActUnfComUsrTL,
   [TL_Frm_ALL_SHA_NOTE] = ActAllShaNotUsrTL,
   [TL_Frm_ALL_FAV_NOTE] = ActAllFavNotUsrTL,
   [TL_Frm_ALL_FAV_COMM] = ActAllFavComUsrTL,
   [TL_Frm_SHO_HID_COMM] = ActShoHidComUsrTL,
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
   struct TL_Form Form =
     {
      .Action      = Action,
      .ParamFormat = ParamFormat,
      .ParamCod    = ParamCod,
      .Icon        = TL_Frm_ICON_ELLIPSIS,
      .Title       = Txt_View_all_USERS,
     };

   switch (HowManyUsrs)
     {
      case TL_Usr_SHOW_FEW_USRS:
	 /***** Form and icon to view all users *****/
	 TL_Frm_FormFavSha (&Form);
	 break;
      case TL_Usr_SHOW_ALL_USRS:
	 /***** Disabled icon *****/
         Ico_PutIconOff (TL_Frm_ICON_ELLIPSIS,Txt_View_all_USERS);
	 break;
     }
  }

/*****************************************************************************/
/******* Form to fav/unfav or share/unshare in global or user timeline *******/
/*****************************************************************************/

void TL_Frm_FormFavSha (const struct TL_Form *Form)
  {
   char *OnSubmit;
   char ParamStr[7 + Cns_MAX_DECIMAL_DIGITS_LONG + 1];

   /***** Create parameter string *****/
   sprintf (ParamStr,Form->ParamFormat,Form->ParamCod);

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
		    Act_GetActCod (TL_Frm_ActionUsr[Form->Action]),
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
		    Act_GetActCod (TL_Frm_ActionGbl[Form->Action]),
		    Gbl.Session.Id,
		    ParamStr) < 0)
	 Lay_NotEnoughMemoryExit ();
      Frm_StartFormUniqueAnchorOnSubmit (ActUnk,NULL,OnSubmit);
     }
   Ico_PutIconLink (Form->Icon,Form->Title);
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

/*****************************************************************************/
/********************** Form to remove note / comment ************************/
/*****************************************************************************/

void TL_Frm_BeginAlertRemove (const char *QuestionTxt)
  {
   Ale_ShowAlertAndButton1 (Ale_QUESTION,QuestionTxt);
  }

void TL_Frm_EndAlertRemove (struct TL_Timeline *Timeline,TL_Frm_Action_t Action,
                            void (*FuncParams) (void *Args))
  {
   extern const char *Txt_Remove;

   if (Gbl.Usrs.Other.UsrDat.UsrCod > 0)
      Ale_ShowAlertAndButton2 (TL_Frm_ActionUsr[Action],"timeline",NULL,
			       FuncParams,Timeline,
			       Btn_REMOVE_BUTTON,Txt_Remove);
   else
      Ale_ShowAlertAndButton2 (TL_Frm_ActionGbl[Action],NULL,NULL,
			       FuncParams,Timeline,
			       Btn_REMOVE_BUTTON,Txt_Remove);
  }
