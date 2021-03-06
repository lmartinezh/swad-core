// swad_timeline_share.c: social timeline shared

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

#include "swad_database.h"
#include "swad_global.h"
#include "swad_timeline.h"
#include "swad_timeline_form.h"
#include "swad_timeline_notification.h"
#include "swad_timeline_publication.h"
#include "swad_timeline_share.h"

/*****************************************************************************/
/****************************** Public constants *****************************/
/*****************************************************************************/

/*****************************************************************************/
/************************* Private constants and types ***********************/
/*****************************************************************************/

#define TL_Sha_ICON_SHARE	"share-alt.svg"
#define TL_Sha_ICON_SHARED	"share-alt-green.svg"

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

static void TL_Sha_ShaNote (struct TL_Not_Note *Not);
static void TL_Sha_UnsNote (struct TL_Not_Note *Not);

static void TL_Sha_PutDisabledIconShare (unsigned NumShared);
static void TL_Sha_PutFormToShaUnsNote (long NotCod);

static bool TL_Sha_CheckIfNoteIsSharedByUsr (long NotCod,long UsrCod);

static void TL_Sha_ShowUsrsWhoHaveSharedNote (const struct TL_Not_Note *Not,
					      TL_Usr_HowManyUsrs_t HowManyUsrs);

/*****************************************************************************/
/******************************** Share a note *******************************/
/*****************************************************************************/

void TL_Sha_ShowAllSharersNoteUsr (void)
  {
   /***** Get user whom profile is displayed *****/
   Usr_GetParamOtherUsrCodEncryptedAndGetUsrData ();

   /***** Show all sharers *****/
   TL_Sha_ShowAllSharersNoteGbl ();
  }

void TL_Sha_ShowAllSharersNoteGbl (void)
  {
   struct TL_Not_Note Not;

   /***** Get data of note *****/
   Not.NotCod = TL_Not_GetParamNotCod ();
   TL_Not_GetDataOfNoteByCod (&Not);

   /***** Write HTML inside DIV with form to share/unshare *****/
   TL_Sha_PutIconToShaUnsNote (&Not,TL_Usr_SHOW_ALL_USRS);
  }

void TL_Sha_ShaNoteUsr (void)
  {
   /***** Get user whom profile is displayed *****/
   Usr_GetParamOtherUsrCodEncryptedAndGetUsrData ();

   /***** Share note *****/
   TL_Sha_ShaNoteGbl ();
  }

void TL_Sha_ShaNoteGbl (void)
  {
   struct TL_Not_Note Not;

   /***** Share note *****/
   TL_Sha_ShaNote (&Not);

   /***** Write HTML inside DIV with form to unshare *****/
   TL_Sha_PutIconToShaUnsNote (&Not,TL_Usr_SHOW_FEW_USRS);
  }

static void TL_Sha_ShaNote (struct TL_Not_Note *Not)
  {
   struct TL_Pub_Publication Pub;
   long OriginalPubCod;

   /***** Get data of note *****/
   Not->NotCod = TL_Not_GetParamNotCod ();
   TL_Not_GetDataOfNoteByCod (Not);

   if (Not->NotCod > 0)
      if (Gbl.Usrs.Me.Logged &&		// I am logged...
	  !Usr_ItsMe (Not->UsrCod))	// ...but I am not the author
         if (!TL_Sha_CheckIfNoteIsSharedByUsr (Not->NotCod,
					       Gbl.Usrs.Me.UsrDat.UsrCod))	// Not yet shared by me
	   {
	    /***** Share (publish note in timeline) *****/
	    Pub.NotCod       = Not->NotCod;
	    Pub.PublisherCod = Gbl.Usrs.Me.UsrDat.UsrCod;
	    Pub.PubType      = TL_Pub_SHARED_NOTE;
	    TL_Pub_PublishPubInTimeline (&Pub);	// Set Pub.PubCod

	    /* Update number of times this note is shared */
	    TL_Sha_UpdateNumTimesANoteHasBeenShared (Not);

	    /**** Create notification about shared post
		  for the author of the post ***/
	    OriginalPubCod = TL_Not_GetPubCodOfOriginalNote (Not->NotCod);
	    if (OriginalPubCod > 0)
	       TL_Ntf_CreateNotifToAuthor (Not->UsrCod,OriginalPubCod,
	                                   Ntf_EVENT_TIMELINE_SHARE);
	   }
  }

/*****************************************************************************/
/******************** Unshare a previously shared note ***********************/
/*****************************************************************************/

void TL_Sha_UnsNoteUsr (void)
  {
   /***** Get user whom profile is displayed *****/
   Usr_GetParamOtherUsrCodEncryptedAndGetUsrData ();

   /***** Unshare note *****/
   TL_Sha_UnsNoteGbl ();
  }

void TL_Sha_UnsNoteGbl (void)
  {
   struct TL_Not_Note Not;

   /***** Unshare note *****/
   TL_Sha_UnsNote (&Not);

   /***** Write HTML inside DIV with form to share *****/
   TL_Sha_PutIconToShaUnsNote (&Not,TL_Usr_SHOW_FEW_USRS);
  }

static void TL_Sha_UnsNote (struct TL_Not_Note *Not)
  {
   long OriginalPubCod;

   /***** Get data of note *****/
   Not->NotCod = TL_Not_GetParamNotCod ();
   TL_Not_GetDataOfNoteByCod (Not);

   if (Not->NotCod > 0)
      if (Not->NumShared &&
	  Gbl.Usrs.Me.Logged &&		// I am logged...
	  !Usr_ItsMe (Not->UsrCod))	// ...but I am not the author
	 if (TL_Sha_CheckIfNoteIsSharedByUsr (Not->NotCod,
					   Gbl.Usrs.Me.UsrDat.UsrCod))	// I am a sharer
	   {
	    /***** Delete publication from database *****/
	    DB_QueryDELETE ("can not remove a publication",
			    "DELETE FROM tl_pubs"
	                    " WHERE NotCod=%ld"
	                    " AND PublisherCod=%ld"
	                    " AND PubType=%u",
			    Not->NotCod,
			    Gbl.Usrs.Me.UsrDat.UsrCod,
			    (unsigned) TL_Pub_SHARED_NOTE);

	    /***** Update number of times this note is shared *****/
	    TL_Sha_UpdateNumTimesANoteHasBeenShared (Not);

            /***** Mark possible notifications on this note as removed *****/
	    OriginalPubCod = TL_Not_GetPubCodOfOriginalNote (Not->NotCod);
	    if (OriginalPubCod > 0)
	       Ntf_MarkNotifAsRemoved (Ntf_EVENT_TIMELINE_SHARE,OriginalPubCod);
	   }
  }

void TL_Sha_PutIconToShaUnsNote (const struct TL_Not_Note *Not,
                                 TL_Usr_HowManyUsrs_t HowManyUsrs)
  {
   /***** Put form to share/unshare this note *****/
   HTM_DIV_Begin ("class=\"TL_ICO\"");
   if (Not->Unavailable ||		// Unavailable notes can not be shared
       Usr_ItsMe (Not->UsrCod))		// I am the author
      /* Put disabled icon */
      TL_Sha_PutDisabledIconShare (Not->NumShared);
   else					// Available and I am not the author
      /* Put icon to share/unshare */
      TL_Sha_PutFormToShaUnsNote (Not->NotCod);
   HTM_DIV_End ();

   /***** Show who have shared this note *****/
   TL_Sha_ShowUsrsWhoHaveSharedNote (Not,HowManyUsrs);
  }

/*****************************************************************************/
/*********************** Put disabled icon to share **************************/
/*****************************************************************************/

static void TL_Sha_PutDisabledIconShare (unsigned NumShared)
  {
   extern const char *Txt_TIMELINE_Shared_by_X_USERS;
   extern const char *Txt_TIMELINE_Not_shared_by_anyone;

   /***** Disabled icon to share *****/
   if (NumShared)
     {
      Ico_PutDivIcon ("TL_ICO_DISABLED",TL_Sha_ICON_SHARE,
		      Str_BuildStringLong (Txt_TIMELINE_Shared_by_X_USERS,
					   (long) NumShared));
      Str_FreeString ();
     }
   else
      Ico_PutDivIcon ("TL_ICO_DISABLED",TL_Sha_ICON_SHARE,
		      Txt_TIMELINE_Not_shared_by_anyone);
  }

/*****************************************************************************/
/*********************** Form to share/unshare note **************************/
/*****************************************************************************/

static void TL_Sha_PutFormToShaUnsNote (long NotCod)
  {
   extern const char *Txt_TIMELINE_Shared;
   extern const char *Txt_TIMELINE_Share;
   struct TL_Form Form[2] =
     {
      [false] = // I have not shared ==> share
        {
         .Action      = TL_Frm_SHA_NOTE,
         .ParamFormat = "NotCod=%ld",
         .ParamCod    = NotCod,
         .Icon        = TL_Sha_ICON_SHARE,
         .Title       = Txt_TIMELINE_Share,
        },
      [true] = // I have shared ==> unshare
	{
	 .Action      = TL_Frm_UNS_NOTE,
	 .ParamFormat = "NotCod=%ld",
	 .ParamCod    = NotCod,
	 .Icon        = TL_Sha_ICON_SHARED,
	 .Title       = Txt_TIMELINE_Shared,
	},
     };

   /***** Form and icon to share/unshare note *****/
   TL_Frm_FormFavSha (&Form[TL_Sha_CheckIfNoteIsSharedByUsr (NotCod,Gbl.Usrs.Me.UsrDat.UsrCod)]);
  }

/*****************************************************************************/
/****************** Check if a user has published a note *********************/
/*****************************************************************************/

static bool TL_Sha_CheckIfNoteIsSharedByUsr (long NotCod,long UsrCod)
  {
   return (DB_QueryCOUNT ("can not check if a user has shared a note",
			  "SELECT COUNT(*) FROM tl_pubs"
			  " WHERE NotCod=%ld"
			  " AND PublisherCod=%ld"
			  " AND PubType=%u",
			  NotCod,
			  UsrCod,
			  (unsigned) TL_Pub_SHARED_NOTE) != 0);
  }

/*****************************************************************************/
/********** Get number of times a note has been shared in timeline ***********/
/*****************************************************************************/

void TL_Sha_UpdateNumTimesANoteHasBeenShared (struct TL_Not_Note *Not)
  {
   /***** Get number of times (users) this note has been shared *****/
   Not->NumShared =
   (unsigned) DB_QueryCOUNT ("can not get number of times"
			     " a note has been shared",
			     "SELECT COUNT(*) FROM tl_pubs"
			     " WHERE NotCod=%ld"
			     " AND PublisherCod<>%ld"
			     " AND PubType=%u",
			     Not->NotCod,
			     Not->UsrCod,	// The author
			     (unsigned) TL_Pub_SHARED_NOTE);
  }

/*****************************************************************************/
/******************* Show users who have shared this note ********************/
/*****************************************************************************/

static void TL_Sha_ShowUsrsWhoHaveSharedNote (const struct TL_Not_Note *Not,
					      TL_Usr_HowManyUsrs_t HowManyUsrs)
  {
   MYSQL_RES *mysql_res;
   unsigned NumFirstUsrs;

   /***** Get users who have shared this note *****/
   if (Not->NumShared)
      NumFirstUsrs =
      (unsigned) DB_QuerySELECT (&mysql_res,"can not get users",
				 "SELECT PublisherCod FROM tl_pubs"
				 " WHERE NotCod=%ld"
				 " AND PublisherCod<>%ld"
				 " AND PubType=%u"
				 " ORDER BY PubCod LIMIT %u",
				 Not->NotCod,
				 Not->UsrCod,
				 (unsigned) TL_Pub_SHARED_NOTE,
				 HowManyUsrs == TL_Usr_SHOW_FEW_USRS ? TL_Usr_DEF_USRS_SHOWN :
				                                       TL_Usr_MAX_USRS_SHOWN);
   else
      NumFirstUsrs = 0;

   /***** Show users *****/
   /* Number of users */
   HTM_DIV_Begin ("class=\"TL_NUM_USRS\"");
   TL_Usr_ShowNumSharersOrFavers (Not->NumShared);
   HTM_DIV_End ();

   /* List users one by one */
   HTM_DIV_Begin ("class=\"TL_USRS\"");
   TL_Usr_ShowSharersOrFavers (&mysql_res,Not->NumShared,NumFirstUsrs);
   if (NumFirstUsrs < Not->NumShared)
      /* Clickable ellipsis to show all users */
      TL_Frm_PutFormToSeeAllFaversSharers (TL_Frm_ALL_SHA_NOTE,
		                           "NotCod=%ld",Not->NotCod,
                                           HowManyUsrs);
   HTM_DIV_End ();

   /***** Free structure that stores the query result *****/
   if (Not->NumShared)
      DB_FreeMySQLResult (&mysql_res);
  }
