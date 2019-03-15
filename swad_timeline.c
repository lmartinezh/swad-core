// swad_timeline.c: social timeline

/*
    SWAD (Shared Workspace At a Distance),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2019 Antonio Ca�as Vargas

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
#include <linux/limits.h>	// For PATH_MAX
#include <stdio.h>		// For asprintf
#include <stdlib.h>		// For malloc and free
#include <string.h>		// For string functions
#include <sys/types.h>		// For time_t

#include "swad_announcement.h"
#include "swad_box.h"
#include "swad_constant.h"
#include "swad_database.h"
#include "swad_exam.h"
#include "swad_follow.h"
#include "swad_form.h"
#include "swad_global.h"
#include "swad_layout.h"
#include "swad_media.h"
#include "swad_notice.h"
#include "swad_notification.h"
#include "swad_parameter.h"
#include "swad_preference.h"
#include "swad_profile.h"
#include "swad_timeline.h"

/*****************************************************************************/
/****************************** Public constants *****************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Private constants *****************************/
/*****************************************************************************/

#define TL_NUM_VISIBLE_COMMENTS		3	// Maximum number of comments visible before expanding them

#define TL_MAX_SHARERS_FAVERS_SHOWN	5	// Maximum number of users shown who have share/fav a note

#define TL_MAX_CHARS_IN_POST		1000

typedef enum
  {
   TL_TIMELINE_USR,	// Show the timeline of a user
   TL_TIMELINE_GBL,	// Show the timeline of the users follwed by me
  } TL_TimelineUsrOrGbl_t;

typedef enum
  {
   TL_GET_ONLY_NEW_PUBS,	// New publications are retrieved via AJAX
				// automatically from time to time
   TL_GET_RECENT_TIMELINE,	// Recent timeline is shown when user clicks on action menu,...
				// or after editing timeline
   TL_GET_ONLY_OLD_PUBS,	// Old publications are retrieved via AJAX
				// when user clicks on link at bottom of timeline
  } TL_WhatToGetFromTimeline_t;

// Timeline images will be saved with:
// - maximum width of TL_IMAGE_SAVED_MAX_HEIGHT
// - maximum height of TL_IMAGE_SAVED_MAX_HEIGHT
// - maintaining the original aspect ratio (aspect ratio recommended: 3:2)
#define TL_IMAGE_SAVED_MAX_WIDTH	768
#define TL_IMAGE_SAVED_MAX_HEIGHT	512
#define TL_IMAGE_SAVED_QUALITY		 75	// 1 to 100
// in timeline posts, the quality should not be high in order to speed up the loading of images

/*****************************************************************************/
/****************************** Internal types *******************************/
/*****************************************************************************/

struct TL_Note
  {
   long NotCod;
   TL_NoteType_t NoteType;
   long UsrCod;
   long HieCod;		// Hierarchy code (institution/centre/degree/course)
   long Cod;		// Code of file, forum post, notice,...
   bool Unavailable;	// File, forum post, notice,... unavailable (removed)
   time_t DateTimeUTC;
   unsigned NumShared;	// Number of times (users) this note has been shared
   unsigned NumFavs;	// Number of times (users) this note has been favourited
  };

struct TL_Comment
  {
   long PubCod;
   long UsrCod;
   long NotCod;		// Note code to which this comment belongs
   time_t DateTimeUTC;
   unsigned NumFavs;	// Number of times (users) this comment has been favourited
   char Content[Cns_MAX_BYTES_LONG_TEXT + 1];
   struct Media Media;
  };

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/************************* Internal global variables *************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Private prototypes ****************************/
/*****************************************************************************/

static void TL_ShowTimelineGblHighlightingNot (long NotCod);
static void TL_ShowTimelineUsrHighlightingNot (long NotCod);

static void TL_GetAndShowOldTimeline (TL_TimelineUsrOrGbl_t TimelineUsrOrGbl);

static void TL_BuildQueryToGetTimeline (char **Query,
                                        TL_TimelineUsrOrGbl_t TimelineUsrOrGbl,
                                        TL_WhatToGetFromTimeline_t WhatToGetFromTimeline);
static long TL_GetPubCodFromSession (const char *FieldName);
static void TL_UpdateLastPubCodIntoSession (void);
static void TL_UpdateFirstPubCodIntoSession (long FirstPubCod);
static void TL_DropTemporaryTablesUsedToQueryTimeline (void);

static void TL_ShowTimeline (char *Query,
                             const char *Title,long NotCodToHighlight);
static void TL_PutIconsTimeline (void);

static void TL_FormStart (Act_Action_t ActionGbl,Act_Action_t ActionUsr);
static void TL_FormFavSha (Act_Action_t ActionGbl,Act_Action_t ActionUsr,
			   const char *ParamCod,
			   const char *Icon,const char *Title);

static void TL_PutFormWhichUsrs (void);
static void TL_PutParamWhichUsrs (void);
static void TL_GetParamsWhichUsrs (void);
static TL_WhichUsrs_t TL_GetWhichUsrsFromDB (void);
static void TL_SaveWhichUsersInDB (void);

static void TL_ShowWarningYouDontFollowAnyUser (void);

static void TL_InsertNewPubsInTimeline (char *Query);
static void TL_ShowOldPubsInTimeline (char *Query);

static void TL_GetDataOfPublicationFromRow (MYSQL_ROW row,struct TL_Publication *SocPub);

static void TL_PutLinkToViewNewPublications (void);
static void TL_PutLinkToViewOldPublications (void);

static void TL_WriteNote (const struct TL_Note *SocNot,
                          TL_TopMessage_t TopMessage,long UsrCod,
                          bool Highlight,
                          bool ShowNoteAlone);
static void TL_WriteTopMessage (TL_TopMessage_t TopMessage,long UsrCod);
static void TL_WriteAuthorNote (const struct UsrData *UsrDat);
static void TL_WriteDateTime (time_t TimeUTC);
static void TL_GetAndWritePost (long PstCod);
static void TL_PutFormGoToAction (const struct TL_Note *SocNot);
static void TL_GetNoteSummary (const struct TL_Note *SocNot,
                               char SummaryStr[Ntf_MAX_BYTES_SUMMARY + 1]);
static void TL_PublishNoteInTimeline (struct TL_Publication *SocPub);

static void TL_PutFormToWriteNewPost (void);
static void TL_PutTextarea (const char *Placeholder,
                            const char *ClassTextArea,const char *ClassImgTit);

static long TL_ReceivePost (void);

static void TL_PutIconToToggleCommentNote (const char UniqueId[Frm_MAX_BYTES_ID + 1]);
static void TL_PutIconCommentDisabled (void);
static void TL_PutHiddenFormToWriteNewCommentToNote (long NotCod,
                                                     const char IdNewComment[Frm_MAX_BYTES_ID + 1]);
static unsigned long TL_GetNumCommentsInNote (long NotCod);
static void TL_WriteCommentsInNote (const struct TL_Note *SocNot);
static void TL_WriteOneCommentInList (MYSQL_RES *mysql_res);
static void TL_PutIconToToggleComments (const char *UniqueId,
                                        const char *Icon,const char *Text);
static void TL_WriteComment (struct TL_Comment *SocCom,
                             TL_TopMessage_t TopMessage,long UsrCod,
                             bool ShowCommentAlone);
static void TL_WriteAuthorComment (struct UsrData *UsrDat);

static void TL_PutFormToRemoveComment (long PubCod);

static void TL_PutDisabledIconShare (unsigned NumShared);
static void TL_PutDisabledIconFav (unsigned NumFavs);

static void TL_PutFormToShareNote (const struct TL_Note *SocNot);
static void TL_PutFormToUnshareNote (const struct TL_Note *SocNot);
static void TL_PutFormToFavNote (const struct TL_Note *SocNot);
static void TL_PutFormToUnfavNote (const struct TL_Note *SocNot);
static void TL_PutFormToFavComment (struct TL_Comment *SocCom);
static void TL_PutFormToUnfavComment (struct TL_Comment *SocCom);

static void TL_PutFormToRemovePublication (long NotCod);

static void TL_PutHiddenParamNotCod (long NotCod);
static long TL_GetParamNotCod (void);
static long TL_GetParamPubCod (void);

static long TL_ReceiveComment (void);

static void TL_ShareNote (struct TL_Note *SocNot);
static void TL_FavNote (struct TL_Note *SocNot);
static void TL_FavComment (struct TL_Comment *SocCom);
static void TL_CreateNotifToAuthor (long AuthorCod,long PubCod,
                                    Ntf_NotifyEvent_t NotifyEvent);

static void TL_UnshareNote (struct TL_Note *SocNot);
static void TL_UnfavNote (struct TL_Note *SocNot);
static void TL_UnfavComment (struct TL_Comment *SocCom);

static void TL_RequestRemovalNote (void);
static void TL_PutParamsRemoveNote (void);
static void TL_RemoveNote (void);
static void TL_RemoveImgFileFromPost (long PstCod);
static void TL_RemoveANoteFromDB (struct TL_Note *SocNot);

static long TL_GetNotCodOfPublication (long PubCod);
static long TL_GetPubCodOfOriginalNote (long NotCod);

static void TL_RequestRemovalComment (void);
static void TL_PutParamsRemoveCommment (void);
static void TL_RemoveComment (void);
static void TL_RemoveImgFileFromComment (long PubCod);
static void TL_RemoveACommentFromDB (struct TL_Comment *SocCom);

static bool TL_CheckIfNoteIsSharedByUsr (long NotCod,long UsrCod);
static bool TL_CheckIfNoteIsFavedByUsr (long NotCod,long UsrCod);
static bool TL_CheckIfCommIsFavedByUsr (long PubCod,long UsrCod);

static void TL_UpdateNumTimesANoteHasBeenShared (struct TL_Note *SocNot);
static void TL_GetNumTimesANoteHasBeenFav (struct TL_Note *SocNot);
static void TL_GetNumTimesACommHasBeenFav (struct TL_Comment *SocCom);

static void TL_ShowUsrsWhoHaveSharedNote (const struct TL_Note *SocNot);
static void TL_ShowUsrsWhoHaveMarkedNoteAsFav (const struct TL_Note *SocNot);
static void TL_ShowUsrsWhoHaveMarkedCommAsFav (const struct TL_Comment *SocCom);
static void TL_ShowSharersOrFavers (MYSQL_RES **mysql_res,
				    unsigned NumUsrs,unsigned NumFirstUsrs);

static void TL_GetDataOfNoteByCod (struct TL_Note *SocNot);
static void TL_GetDataOfCommByCod (struct TL_Comment *SocCom);

static void TL_GetDataOfPublicationFromRow (MYSQL_ROW row,struct TL_Publication *SocPub);
static void TL_GetDataOfNoteFromRow (MYSQL_ROW row,struct TL_Note *SocNot);
static TL_PubType_t TL_GetPubTypeFromStr (const char *Str);
static TL_NoteType_t TL_GetNoteTypeFromStr (const char *Str);
static void TL_GetDataOfCommentFromRow (MYSQL_ROW row,struct TL_Comment *SocCom);

static void TL_ResetNote (struct TL_Note *SocNot);
static void TL_ResetComment (struct TL_Comment *SocCom);

static void TL_ClearTimelineThisSession (void);
static void TL_AddNotesJustRetrievedToTimelineThisSession (void);

static void Str_AnalyzeTxtAndStoreNotifyEventToMentionedUsrs (long PubCod,const char *Txt);

/*****************************************************************************/
/************** Show timeline including all the users I follow ***************/
/*****************************************************************************/

void TL_ShowTimelineGbl1 (void)
  {
   /***** Mark all my notifications about timeline as seen *****/
   TL_MarkMyNotifAsSeen ();

   /***** Get which users *****/
   TL_GetParamsWhichUsrs ();

   /***** Save which users in database *****/
   if (Gbl.Action.Act == ActSeeSocTmlGbl)	// Only in action to see global timeline
      TL_SaveWhichUsersInDB ();
  }

void TL_ShowTimelineGbl2 (void)
  {
   long PubCod;
   struct TL_Note SocNot;
   struct UsrData UsrDat;
   Ntf_NotifyEvent_t NotifyEvent;
   const TL_TopMessage_t TopMessages[Ntf_NUM_NOTIFY_EVENTS] =
     {
      TL_TOP_MESSAGE_NONE,	// Ntf_EVENT_UNKNOWN

      /* Course tab */
      TL_TOP_MESSAGE_NONE,	// Ntf_EVENT_DOCUMENT_FILE
      TL_TOP_MESSAGE_NONE,	// Ntf_EVENT_TEACHERS_FILE
      TL_TOP_MESSAGE_NONE,	// Ntf_EVENT_SHARED_FILE

      /* Assessment tab */
      TL_TOP_MESSAGE_NONE,	// Ntf_EVENT_ASSIGNMENT
      TL_TOP_MESSAGE_NONE,	// Ntf_EVENT_EXAM_ANNOUNCEMENT
      TL_TOP_MESSAGE_NONE,	// Ntf_EVENT_MARKS_FILE

      /* Users tab */
      TL_TOP_MESSAGE_NONE,	// Ntf_EVENT_ENROLMENT_STD
      TL_TOP_MESSAGE_NONE,	// Ntf_EVENT_ENROLMENT_TCH
      TL_TOP_MESSAGE_NONE,	// Ntf_EVENT_ENROLMENT_REQUEST

      /* Start tab */	// TODO: Move to top
      TL_TOP_MESSAGE_COMMENTED,	// Ntf_EVENT_TIMELINE_COMMENT
      TL_TOP_MESSAGE_FAVED,	// Ntf_EVENT_TIMELINE_FAV
      TL_TOP_MESSAGE_SHARED,	// Ntf_EVENT_TIMELINE_SHARE
      TL_TOP_MESSAGE_MENTIONED,	// Ntf_EVENT_TIMELINE_MENTION
      TL_TOP_MESSAGE_NONE,	// Ntf_EVENT_FOLLOWER

      /* Messages tab */
      TL_TOP_MESSAGE_NONE,	// Ntf_EVENT_FORUM_POST_COURSE
      TL_TOP_MESSAGE_NONE,	// Ntf_EVENT_FORUM_REPLY
      TL_TOP_MESSAGE_NONE,	// Ntf_EVENT_NOTICE
      TL_TOP_MESSAGE_NONE,	// Ntf_EVENT_MESSAGE

      /* Statistics tab */

      /* Profile tab */

      TL_TOP_MESSAGE_NONE,	// Ntf_EVENT_SURVEY		// TODO: Move to assessment tab (also necessary in database) !!!!!!!!!
      TL_TOP_MESSAGE_NONE,	// Ntf_EVENT_ENROLMENT_NET	// TODO: Move to users tab (also necessary in database) !!!!!!!!!
     };

   /***** If I am been redirected from another action... *****/
   switch (Gbl.Action.Original)
     {
      case ActLogIn:
      case ActLogInLan:
         Usr_WelcomeUsr ();
         break;
      case ActAnnSee:
	 Ann_MarkAnnouncementAsSeen ();
	 break;
      default:
         break;
     }

   /***** Initialize note code to -1 ==> no highlighted note *****/
   SocNot.NotCod = -1L;

   /***** Get parameter with the code of a publication *****/
   // This parameter is optional. It can be provided by a notification.
   // If > 0 ==> the note is shown highlighted above the timeline
   PubCod = TL_GetParamPubCod ();
   if (PubCod > 0)
      /***** Get code of note from database *****/
      SocNot.NotCod = TL_GetNotCodOfPublication (PubCod);

   if (SocNot.NotCod > 0)
     {
      /* Get who did the action (publishing, commenting, faving, sharing, mentioning) */
      Usr_GetParamOtherUsrCodEncrypted (&UsrDat);

      /* Get what he/she did */
      NotifyEvent = Ntf_GetParamNotifyEvent ();

      /***** Show the note highlighted *****/
      TL_GetDataOfNoteByCod (&SocNot);
      TL_WriteNote (&SocNot,
		    TopMessages[NotifyEvent],UsrDat.UsrCod,
		    true,true);
     }

   /***** Show timeline with possible highlighted note *****/
   TL_ShowTimelineGblHighlightingNot (SocNot.NotCod);
  }

static void TL_ShowTimelineGblHighlightingNot (long NotCod)
  {
   extern const char *Txt_Timeline;
   char *Query = NULL;

   /***** Build query to get timeline *****/
   TL_BuildQueryToGetTimeline (&Query,
	                       TL_TIMELINE_GBL,
                               TL_GET_RECENT_TIMELINE);

   /***** Show timeline *****/
   TL_ShowTimeline (Query,Txt_Timeline,NotCod);

   /***** Drop temporary tables *****/
   TL_DropTemporaryTablesUsedToQueryTimeline ();
  }

/*****************************************************************************/
/********************* Show timeline of a selected user **********************/
/*****************************************************************************/

void TL_ShowTimelineUsr (void)
  {
   TL_ShowTimelineUsrHighlightingNot (-1L);
  }

static void TL_ShowTimelineUsrHighlightingNot (long NotCod)
  {
   extern const char *Txt_Timeline_OF_A_USER;
   char *Query = NULL;

   /***** Build query to show timeline with publications of a unique user *****/
   TL_BuildQueryToGetTimeline (&Query,
	                       TL_TIMELINE_USR,
                               TL_GET_RECENT_TIMELINE);

   /***** Show timeline *****/
   snprintf (Gbl.Title,sizeof (Gbl.Title),
	     Txt_Timeline_OF_A_USER,
	     Gbl.Usrs.Other.UsrDat.FirstName);
   TL_ShowTimeline (Query,Gbl.Title,NotCod);

   /***** Drop temporary tables *****/
   TL_DropTemporaryTablesUsedToQueryTimeline ();
  }

/*****************************************************************************/
/************** Refresh new publications in timeline via AJAX ****************/
/*****************************************************************************/

void TL_RefreshNewTimelineGbl (void)
  {
   char *Query = NULL;

   if (Gbl.Session.IsOpen)	// If session has been closed, do not write anything
     {
      /***** Get which users *****/
      TL_GetParamsWhichUsrs ();

      /***** Build query to get timeline *****/
      TL_BuildQueryToGetTimeline (&Query,
	                          TL_TIMELINE_GBL,
				  TL_GET_ONLY_NEW_PUBS);

      /***** Show new timeline *****/
      TL_InsertNewPubsInTimeline (Query);

      /***** Drop temporary tables *****/
      TL_DropTemporaryTablesUsedToQueryTimeline ();
     }

   /***** All the output is made, so don't write anymore *****/
   Gbl.Layout.DivsEndWritten = Gbl.Layout.HTMLEndWritten = true;
  }

/*****************************************************************************/
/**************** View old publications in timeline via AJAX *****************/
/*****************************************************************************/

void TL_RefreshOldTimelineGbl (void)
  {
   /***** Get which users *****/
   TL_GetParamsWhichUsrs ();

   /***** Show old publications *****/
   TL_GetAndShowOldTimeline (TL_TIMELINE_GBL);
  }

void TL_RefreshOldTimelineUsr (void)
  {
   /***** Get user whom profile is displayed *****/
   if (Usr_GetParamOtherUsrCodEncryptedAndGetUsrData ())	// Existing user
      /***** If user exists, show old publications *****/
      TL_GetAndShowOldTimeline (TL_TIMELINE_USR);
  }

/*****************************************************************************/
/**************** Get and show old publications in timeline ******************/
/*****************************************************************************/

static void TL_GetAndShowOldTimeline (TL_TimelineUsrOrGbl_t TimelineUsrOrGbl)
  {
   char *Query = NULL;

   /***** Build query to get timeline *****/
   TL_BuildQueryToGetTimeline (&Query,
	                       TimelineUsrOrGbl,
                               TL_GET_ONLY_OLD_PUBS);

   /***** Show old timeline *****/
   TL_ShowOldPubsInTimeline (Query);

   /***** Drop temporary tables *****/
   TL_DropTemporaryTablesUsedToQueryTimeline ();

   /***** All the output is made, so don't write anymore *****/
   Gbl.Layout.DivsEndWritten = Gbl.Layout.HTMLEndWritten = true;
  }

/*****************************************************************************/
/************ Mark all my notifications about timeline as seen ***************/
/*****************************************************************************/
// Must be executed as a priori function

void TL_MarkMyNotifAsSeen (void)
  {
   Ntf_MarkNotifAsSeen (Ntf_EVENT_TIMELINE_COMMENT,-1L,-1L,Gbl.Usrs.Me.UsrDat.UsrCod);
   Ntf_MarkNotifAsSeen (Ntf_EVENT_TIMELINE_FAV    ,-1L,-1L,Gbl.Usrs.Me.UsrDat.UsrCod);
   Ntf_MarkNotifAsSeen (Ntf_EVENT_TIMELINE_SHARE  ,-1L,-1L,Gbl.Usrs.Me.UsrDat.UsrCod);
   Ntf_MarkNotifAsSeen (Ntf_EVENT_TIMELINE_MENTION,-1L,-1L,Gbl.Usrs.Me.UsrDat.UsrCod);
  }

/*****************************************************************************/
/************************ Build query to get timeline ************************/
/*****************************************************************************/

#define TL_MAX_BYTES_SUBQUERY_ALREADY_EXISTS (256 - 1)

static void TL_BuildQueryToGetTimeline (char **Query,
                                        TL_TimelineUsrOrGbl_t TimelineUsrOrGbl,
                                        TL_WhatToGetFromTimeline_t WhatToGetFromTimeline)
  {
   char SubQueryPublishers[128];
   char SubQueryRangeBottom[128];
   char SubQueryRangeTop[128];
   char SubQueryAlreadyExists[TL_MAX_BYTES_SUBQUERY_ALREADY_EXISTS + 1];
   struct
     {
      long Top;
      long Bottom;
     } RangePubsToGet;
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumPubs;
   unsigned NumPub;
   long PubCod;
   long NotCod;
   const unsigned MaxPubsToGet[3] =
     {
      TL_MAX_NEW_PUBS_TO_GET_AND_SHOW,	// TL_GET_ONLY_NEW_PUBS
      TL_MAX_REC_PUBS_TO_GET_AND_SHOW,	// TL_GET_RECENT_TIMELINE
      TL_MAX_OLD_PUBS_TO_GET_AND_SHOW,	// TL_GET_ONLY_OLD_PUBS
     };

   /***** Clear timeline for this session in database *****/
   if (WhatToGetFromTimeline == TL_GET_RECENT_TIMELINE)
      TL_ClearTimelineThisSession ();

   /***** Drop temporary tables *****/
   TL_DropTemporaryTablesUsedToQueryTimeline ();

   /***** Create temporary table with publication codes *****/
   DB_Query ("can not create temporary table",
	     "CREATE TEMPORARY TABLE pub_codes "
	     "(PubCod BIGINT NOT NULL,UNIQUE INDEX(PubCod)) ENGINE=MEMORY");

   /***** Create temporary table with notes got in this execution *****/
   DB_Query ("can not create temporary table",
	     "CREATE TEMPORARY TABLE not_codes "
	     "(NotCod BIGINT NOT NULL,INDEX(NotCod)) ENGINE=MEMORY");

   /***** Create temporary table with notes already present in timeline for this session *****/
   DB_Query ("can not create temporary table",
	     "CREATE TEMPORARY TABLE current_timeline "
	     "(NotCod BIGINT NOT NULL,INDEX(NotCod)) ENGINE=MEMORY"
	     " SELECT NotCod FROM social_timelines WHERE SessionId='%s'",
	     Gbl.Session.Id);

   /***** Create temporary table and subquery with potential publishers *****/
   switch (TimelineUsrOrGbl)
     {
      case TL_TIMELINE_USR:	// Show the timeline of a user
	 sprintf (SubQueryPublishers,"PublisherCod=%ld AND ",
	          Gbl.Usrs.Other.UsrDat.UsrCod);
	 break;
      case TL_TIMELINE_GBL:	// Show the global timeline
	 switch (Gbl.Timeline.WhichUsrs)
	   {
	    case TL_USRS_FOLLOWED:	// Show the timeline of the users I follow
	       DB_Query ("can not create temporary table",
		         "CREATE TEMPORARY TABLE publishers "
			 "(UsrCod INT NOT NULL,UNIQUE INDEX(UsrCod)) ENGINE=MEMORY"
			 " SELECT %ld AS UsrCod"
			 " UNION"
			 " SELECT FollowedCod AS UsrCod"
			 " FROM usr_follow WHERE FollowerCod=%ld",
			 Gbl.Usrs.Me.UsrDat.UsrCod,
			 Gbl.Usrs.Me.UsrDat.UsrCod);

	       sprintf (SubQueryPublishers,"social_pubs.PublisherCod=publishers.UsrCod AND ");
	       break;
	    case TL_USRS_ALL:	// Show the timeline of all users
	       SubQueryPublishers[0] = '\0';
	       break;
	    default:
	       Lay_ShowErrorAndExit ("Wrong parameter which users.");
	       break;
	   }
	 break;
     }

   /***** Create subquery to get only notes not present in timeline *****/
   switch (TimelineUsrOrGbl)
     {
      case TL_TIMELINE_USR:	// Show the timeline of a user
	 switch (WhatToGetFromTimeline)
           {
            case TL_GET_ONLY_NEW_PUBS:
            case TL_GET_RECENT_TIMELINE:
	       Str_Copy (SubQueryAlreadyExists,
	                 " NotCod NOT IN"
			 " (SELECT NotCod FROM not_codes)",
			 TL_MAX_BYTES_SUBQUERY_ALREADY_EXISTS);
	       break;
            case TL_GET_ONLY_OLD_PUBS:
	       Str_Copy (SubQueryAlreadyExists,
	                 " NotCod NOT IN"
			 " (SELECT NotCod FROM current_timeline)",
			 TL_MAX_BYTES_SUBQUERY_ALREADY_EXISTS);
	       break;
           }
	 break;
      case TL_TIMELINE_GBL:	// Show the timeline of the users I follow
	 switch (WhatToGetFromTimeline)
           {
            case TL_GET_ONLY_NEW_PUBS:
            case TL_GET_RECENT_TIMELINE:
	       Str_Copy (SubQueryAlreadyExists,
	                 " social_pubs.NotCod NOT IN"
			 " (SELECT NotCod FROM not_codes)",
			 TL_MAX_BYTES_SUBQUERY_ALREADY_EXISTS);
	       break;
            case TL_GET_ONLY_OLD_PUBS:
	       Str_Copy (SubQueryAlreadyExists,
	                 " social_pubs.NotCod NOT IN"
			 " (SELECT NotCod FROM current_timeline)",
			 TL_MAX_BYTES_SUBQUERY_ALREADY_EXISTS);
	       break;
           }
	 break;
     }

   /***** Get the publications in timeline *****/
   /* Initialize range of pubs:

            social_pubs
               _____
              |_____|11
              |_____|10
             _|_____| 9 <-- RangePubsToGet.Top
     Get    / |_____| 8
    pubs   |  |_____| 7
    from  <   |_____| 6
    this   |  |_____| 5
   range    \_|_____| 4
              |_____| 3 <-- RangePubsToGet.Bottom
              |_____| 2
              |_____| 1
                      0
   */
   RangePubsToGet.Top    = 0;	// +Infinite
   RangePubsToGet.Bottom = 0;	// -Infinite
   switch (WhatToGetFromTimeline)
     {
      case TL_GET_ONLY_NEW_PUBS:	 // Get the publications (without limit) newer than LastPubCod
	 /* This query is made via AJAX automatically from time to time */
	 RangePubsToGet.Bottom = TL_GetPubCodFromSession ("LastPubCod");
	 break;
      case TL_GET_RECENT_TIMELINE:	 // Get some limited recent publications
	 /* This is the first query to get initial timeline shown
	    ==> no notes yet in current timeline table */
	 break;
      case TL_GET_ONLY_OLD_PUBS:	 // Get some limited publications older than FirstPubCod
	 /* This query is made via AJAX
	    when I click in link to get old publications */
	 RangePubsToGet.Top    = TL_GetPubCodFromSession ("FirstPubCod");
	 break;
     }

   /*
      With the current approach, we select one by one
      the publications and notes in a loop. In each iteration,
      we get the more recent publication (original, shared or commment)
      of every set of publications corresponding to the same note,
      checking that the note is not already retrieved.
      After getting a publication, its note code is stored
      in order to not get it again.

      As an alternative, we tried to get the maximum PubCod,
      i.e more recent publication (original, shared or commment),
      of every set of publications corresponding to the same note:
      "SELECT MAX(PubCod) AS NewestPubCod FROM social_pubs ...
      " GROUP BY NotCod ORDER BY NewestPubCod DESC LIMIT ..."
      but this query is slow (several seconds) with a big table.
    */
   for (NumPub = 0;
	NumPub < MaxPubsToGet[WhatToGetFromTimeline];
	NumPub++)
     {
      /* Create subqueries with range of publications to get from social_pubs */
      if (RangePubsToGet.Bottom > 0)
	 switch (TimelineUsrOrGbl)
	   {
	    case TL_TIMELINE_USR:	// Show the timeline of a user
	       sprintf (SubQueryRangeBottom,"PubCod>%ld AND ",
		        RangePubsToGet.Bottom);
	       break;
	    case TL_TIMELINE_GBL:	// Show the global timeline
	       switch (Gbl.Timeline.WhichUsrs)
		 {
		  case TL_USRS_FOLLOWED:	// Show the timeline of the users I follow
		     sprintf (SubQueryRangeBottom,"social_pubs.PubCod>%ld AND ",
			      RangePubsToGet.Bottom);
		     break;
		  case TL_USRS_ALL:		// Show the timeline of all users
		     sprintf (SubQueryRangeBottom,"PubCod>%ld AND ",
			      RangePubsToGet.Bottom);
		     break;
		  default:
		     Lay_ShowErrorAndExit ("Wrong parameter which users.");
		     break;
		 }
	       break;
	   }
      else
	 SubQueryRangeBottom[0] = '\0';

      if (RangePubsToGet.Top > 0)
	 switch (TimelineUsrOrGbl)
	   {
	    case TL_TIMELINE_USR:	// Show the timeline of a user
	       sprintf (SubQueryRangeTop,"PubCod<%ld AND ",
		        RangePubsToGet.Top);
	       break;
	    case TL_TIMELINE_GBL:	// Show the global timeline
	       switch (Gbl.Timeline.WhichUsrs)
		 {
		  case TL_USRS_FOLLOWED:	// Show the timeline of the users I follow
		     sprintf (SubQueryRangeTop,"social_pubs.PubCod<%ld AND ",
			      RangePubsToGet.Top);
		     break;
		  case TL_USRS_ALL:	// Show the timeline of all users
		     sprintf (SubQueryRangeTop,"PubCod<%ld AND ",
			      RangePubsToGet.Top);
		     break;
		  default:
		     Lay_ShowErrorAndExit ("Wrong parameter which users.");
		     break;
		 }
	       break;
	   }
      else
	 SubQueryRangeTop[0] = '\0';

      /* Select the most recent publication from social_pubs */
      NumPubs = 0;	// Initialized to avoid warning
      switch (TimelineUsrOrGbl)
	{
	 case TL_TIMELINE_USR:	// Show the timeline of a user
	    NumPubs =
	    (unsigned) DB_QuerySELECT (&mysql_res,"can not get publication",
				       "SELECT PubCod,NotCod"
				       " FROM social_pubs"
				       " WHERE %s%s%s%s"
				       " ORDER BY PubCod DESC LIMIT 1",
				       SubQueryRangeBottom,SubQueryRangeTop,
				       SubQueryPublishers,
				       SubQueryAlreadyExists);
	    break;
	 case TL_TIMELINE_GBL:	// Show the global timeline
	    switch (Gbl.Timeline.WhichUsrs)
	      {
	       case TL_USRS_FOLLOWED:	// Show the timeline of the users I follow
		  NumPubs =
		  (unsigned) DB_QuerySELECT (&mysql_res,"can not get publication",
				             "SELECT PubCod,NotCod"
				             " FROM social_pubs,publishers"
					     " WHERE %s%s%s%s"
					     " ORDER BY social_pubs.PubCod DESC LIMIT 1",
					     SubQueryRangeBottom,SubQueryRangeTop,
					     SubQueryPublishers,
					     SubQueryAlreadyExists);
		  break;
	       case TL_USRS_ALL:	// Show the timeline of all users
		  NumPubs =
		  (unsigned) DB_QuerySELECT (&mysql_res,"can not get publication",
				             "SELECT PubCod,NotCod"
				             " FROM social_pubs"
					     " WHERE %s%s%s"
					     " ORDER BY PubCod DESC LIMIT 1",
					     SubQueryRangeBottom,SubQueryRangeTop,
					     SubQueryAlreadyExists);
		  break;
	       default:
		  Lay_ShowErrorAndExit ("Wrong parameter which users.");
		  break;
	      }
	    break;
	}

      if (NumPubs == 1)
	{
	 /* Get code of publication */
	 row = mysql_fetch_row (mysql_res);
	 PubCod = Str_ConvertStrCodToLongCod (row[0]);
	}
      else
        {
	 row = NULL;
	 PubCod = -1L;
        }

      /* Free structure that stores the query result */
      DB_FreeMySQLResult (&mysql_res);

      if (PubCod > 0)
	{
	 DB_QueryINSERT ("can not store publication code",
			 "INSERT INTO pub_codes SET PubCod=%ld",
			 PubCod);
	 RangePubsToGet.Top = PubCod;	// Narrow the range for the next iteration

	 /* Get note code (row[1]) */
	 if (row)
	   {
	    NotCod = Str_ConvertStrCodToLongCod (row[1]);
	    DB_QueryINSERT ("can not store note code",
			    "INSERT INTO not_codes SET NotCod=%ld",
			    NotCod);
	    DB_QueryINSERT ("can not store note code",
			    "INSERT INTO current_timeline SET NotCod=%ld",
			    NotCod);
	   }
	}
      else	// Nothing got ==> abort loop
         break;	// Last publication
     }

   /***** Update last publication code into session for next refresh *****/
   // Do this inmediately after getting the publications codes...
   // ...in order to not lose publications
   TL_UpdateLastPubCodIntoSession ();

   /***** Add notes just retrieved to current timeline for this session *****/
   TL_AddNotesJustRetrievedToTimelineThisSession ();

   /***** Build query to show timeline including the users I am following *****/
   DB_BuildQuery (Query,
	          "SELECT PubCod,"			// row[0]
	                 "NotCod,"			// row[1]
	                 "PublisherCod,"		// row[2]
	                 "PubType,"			// row[3]
	                 "UNIX_TIMESTAMP(TimePublish)"	// row[4]
		  " FROM social_pubs"
		  " WHERE PubCod IN "
		  "(SELECT PubCod"
		  " FROM pub_codes)"
		  " ORDER BY PubCod DESC");
  }

/*****************************************************************************/
/************* Get last/first publication code stored in session *************/
/*****************************************************************************/
// FieldName can be:
// "LastPubCod"
// "FirstPubCod"

static long TL_GetPubCodFromSession (const char *FieldName)
  {
   extern const char *Txt_The_session_has_expired;
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   long PubCod;

   /***** Get last publication code from database *****/
   if (DB_QuerySELECT (&mysql_res,"can not get publication code from session",
		       "SELECT %s FROM sessions"
		       " WHERE SessionId='%s'",
		       FieldName,Gbl.Session.Id) != 1)
      Lay_ShowErrorAndExit (Txt_The_session_has_expired);

   /***** Get last publication code *****/
   row = mysql_fetch_row (mysql_res);
   if (sscanf (row[0],"%ld",&PubCod) != 1)
      PubCod = 0;

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   return PubCod;
  }

/*****************************************************************************/
/*********************** Update last publication code ************************/
/*****************************************************************************/

static void TL_UpdateLastPubCodIntoSession (void)
  {
   /***** Update last publication code *****/
   DB_QueryUPDATE ("can not update last publication code into session",
		   "UPDATE sessions"
	           " SET LastPubCod="
	           "(SELECT IFNULL(MAX(PubCod),0) FROM social_pubs)"
	           " WHERE SessionId='%s'",
		   Gbl.Session.Id);
  }

/*****************************************************************************/
/*********************** Update first publication code ***********************/
/*****************************************************************************/

static void TL_UpdateFirstPubCodIntoSession (long FirstPubCod)
  {
   /***** Update last publication code *****/
   DB_QueryUPDATE ("can not update first publication code into session",
		   "UPDATE sessions SET FirstPubCod=%ld WHERE SessionId='%s'",
		   FirstPubCod,Gbl.Session.Id);
  }

/*****************************************************************************/
/*************** Drop temporary tables used to query timeline ****************/
/*****************************************************************************/

static void TL_DropTemporaryTablesUsedToQueryTimeline (void)
  {
   DB_Query ("can not remove temporary tables",
	     "DROP TEMPORARY TABLE IF EXISTS"
	     " pub_codes,not_codes,publishers,current_timeline");
  }

/*****************************************************************************/
/******************************* Show timeline *******************************/
/*****************************************************************************/
/*             _____
            / |_____| just_now_timeline_list (Posts retrieved automatically
           |  |_____|                         via AJAX from time to time.
           |  |_____|                         They are transferred inmediately
           |     |                            to new_timeline_list.)
  Hidden  <    __v__
           |  |_____| new_timeline_list (Posts retrieved but hidden.
           |  |_____|                    When user clicks to view them,
           |  |_____|                    they are transferred
            \ |_____|                    to visible timeline_list.)
                 |
               __v__
            / |_____| timeline_list (Posts visible on page)
           |  |_____|
  Visible  |  |_____|
    on    <   |_____|
   page    |  |_____|
           |  |_____|
            \ |_____|
                 ^
               __|__
            / |_____| old_timeline_list (Posts just retrieved via AJAX
           |  |_____|                    when user clicks "see more".
           |  |_____|                    They are transferred inmediately
  Hidden  <   |_____|                    to timeline_list.)
           |  |_____|
           |  |_____|
            \ |_____|
*/
static void TL_ShowTimeline (char *Query,
                             const char *Title,long NotCodToHighlight)
  {
   extern const char *Hlp_START_Timeline;
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned long NumPubsGot;
   unsigned long NumPub;
   struct TL_Publication SocPub;
   struct TL_Note SocNot;
   bool GlobalTimeline = (Gbl.Usrs.Other.UsrDat.UsrCod <= 0);
   bool ItsMe = Usr_ItsMe (Gbl.Usrs.Other.UsrDat.UsrCod);

   /***** Get publications from database *****/
   NumPubsGot = DB_QuerySELECT (&mysql_res,"can not get timeline",
				"%s",
				Query);

   /***** Start box *****/
   Box_StartBox (NULL,Title,TL_PutIconsTimeline,
                 Hlp_START_Timeline,Box_NOT_CLOSABLE);

   /***** Put form to select users whom public activity is displayed *****/
   if (GlobalTimeline)
      TL_PutFormWhichUsrs ();

   /***** Form to write a new post *****/
   if (GlobalTimeline || ItsMe)
      TL_PutFormToWriteNewPost ();

   /***** New publications refreshed dynamically via AJAX *****/
   if (GlobalTimeline)
     {
      /* Link to view new publications via AJAX */
      TL_PutLinkToViewNewPublications ();

      /* Hidden list where insert just received (not visible) publications via AJAX */
      fprintf (Gbl.F.Out,"<ul id=\"just_now_timeline_list\" class=\"TL_LIST\"></ul>");

      /* Hidden list where insert new (not visible) publications via AJAX */
      fprintf (Gbl.F.Out,"<ul id=\"new_timeline_list\" class=\"TL_LIST\"></ul>");
     }

   /***** List recent publications in timeline *****/
   fprintf (Gbl.F.Out,"<ul id=\"timeline_list\" class=\"TL_LIST\">");

   for (NumPub = 0, SocPub.PubCod = 0;
	NumPub < NumPubsGot;
	NumPub++)
     {
      /* Get data of publication */
      row = mysql_fetch_row (mysql_res);
      TL_GetDataOfPublicationFromRow (row,&SocPub);

      /* Get data of note */
      SocNot.NotCod = SocPub.NotCod;
      TL_GetDataOfNoteByCod (&SocNot);

      /* Write note */
      TL_WriteNote (&SocNot,
                    SocPub.TopMessage,SocPub.PublisherCod,
		    SocNot.NotCod == NotCodToHighlight,
		    false);
     }
   fprintf (Gbl.F.Out,"</ul>");

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   /***** Store first publication code into session *****/
   TL_UpdateFirstPubCodIntoSession (SocPub.PubCod);

   if (NumPubsGot == TL_MAX_REC_PUBS_TO_GET_AND_SHOW)
     {
      /***** Link to view old publications via AJAX *****/
      TL_PutLinkToViewOldPublications ();

      /***** Hidden list where insert old publications via AJAX *****/
      fprintf (Gbl.F.Out,"<ul id=\"old_timeline_list\" class=\"TL_LIST\"></ul>");
     }

   /***** End box *****/
   Box_EndBox ();
  }

/*****************************************************************************/
/********************* Put contextual icons in timeline **********************/
/*****************************************************************************/

static void TL_PutIconsTimeline (void)
  {
   /***** Put icon to show a figure *****/
   Gbl.Figures.FigureType = Fig_TIMELINE;
   Fig_PutIconToShowFigure ();
  }

/*****************************************************************************/
/***************** Start a form in global or user timeline *******************/
/*****************************************************************************/

static void TL_FormStart (Act_Action_t ActionGbl,Act_Action_t ActionUsr)
  {
   if (Gbl.Usrs.Other.UsrDat.UsrCod > 0)
     {
      Frm_StartFormAnchor (ActionUsr,"timeline");
      Usr_PutParamOtherUsrCodEncrypted ();
     }
   else
     {
      Frm_StartForm (ActionGbl);
      TL_PutParamWhichUsrs ();
     }
  }

/*****************************************************************************/
/******* Form to fav/unfav or share/unshare in global or user timeline *******/
/*****************************************************************************/

static void TL_FormFavSha (Act_Action_t ActionGbl,Act_Action_t ActionUsr,
			   const char *ParamCod,
			   const char *Icon,const char *Title)
  {
   char *OnSubmit;

   /***** Form and icon to mark note as favourite *****/
   /* Form with icon */
   if (Gbl.Usrs.Other.UsrDat.UsrCod > 0)
     {
      if (asprintf (&OnSubmit,"updateParentDiv(this,"
			      "'act=%ld&ses=%s&%s&OtherUsrCod=%s');"
			      " return false;",	// return false is necessary to not submit form
		    Act_GetActCod (ActionUsr),
		    Gbl.Session.Id,
		    ParamCod,
		    Gbl.Usrs.Other.UsrDat.EncryptedUsrCod) < 0)
	 Lay_NotEnoughMemoryExit ();
      Frm_StartFormUniqueAnchorOnSubmit (ActUnk,"timeline",OnSubmit);
     }
   else
     {
      if (asprintf (&OnSubmit,"updateParentDiv(this,"
			      "'act=%ld&ses=%s&%s');"
			      " return false;",	// return false is necessary to not submit form
		    Act_GetActCod (ActionGbl),
		    Gbl.Session.Id,
		    ParamCod) < 0)
	 Lay_NotEnoughMemoryExit ();
      Frm_StartFormUniqueAnchorOnSubmit (ActUnk,NULL,OnSubmit);
     }
   Ico_PutIconLink (Icon,Title);
   Frm_EndForm ();

   /* Free allocated memory for subquery */
   free ((void *) OnSubmit);
  }

/*****************************************************************************/
/******** Show form to select users whom public activity is displayed ********/
/*****************************************************************************/

static void TL_PutFormWhichUsrs (void)
  {
   extern const char *Txt_TIMELINE_WHICH_USERS[TL_NUM_WHICH_USRS];
   TL_WhichUsrs_t WhichUsrs;
   static const char *Icon[TL_NUM_WHICH_USRS] =
     {
      NULL,		// TL_USRS_UNKNOWN
      "user-check.svg",	// TL_USRS_FOLLOWED
      "users.svg",	// TL_USRS_ALL
     };

   /***** Preference selector for which users *****/
   Pre_StartPrefsHead ();
   Pre_StartOnePrefSelector ();
   for (WhichUsrs = (TL_WhichUsrs_t) 1;
	WhichUsrs < TL_NUM_WHICH_USRS;
	WhichUsrs++)
     {
      fprintf (Gbl.F.Out,"<div class=\"%s\">",
	       WhichUsrs == Gbl.Timeline.WhichUsrs ? "PREF_ON" :
						   "PREF_OFF");
      Frm_StartForm (ActSeeSocTmlGbl);
      Par_PutHiddenParamUnsigned ("WhichUsrs",WhichUsrs);
      Ico_PutPrefIconLink (Icon[WhichUsrs],Txt_TIMELINE_WHICH_USERS[WhichUsrs]);
      Frm_EndForm ();
      fprintf (Gbl.F.Out,"</div>");
     }
   Pre_EndOnePrefSelector ();
   Pre_EndPrefsHead ();

   /***** Show warning if I do not follow anyone *****/
   if (Gbl.Timeline.WhichUsrs == TL_USRS_FOLLOWED)
      TL_ShowWarningYouDontFollowAnyUser ();
  }

/*****************************************************************************/
/***** Put hidden parameter with which users to view in global timeline ******/
/*****************************************************************************/

static void TL_PutParamWhichUsrs (void)
  {
   Par_PutHiddenParamUnsigned ("WhichUsrs",Gbl.Timeline.WhichUsrs);
  }

/*****************************************************************************/
/********* Get parameter with which users to view in global timeline *********/
/*****************************************************************************/

static void TL_GetParamsWhichUsrs (void)
  {
   /***** Get which users I want to see *****/
   Gbl.Timeline.WhichUsrs = (TL_WhichUsrs_t)
	                   Par_GetParToUnsignedLong ("WhichUsrs",
                                                     1,
                                                     TL_NUM_WHICH_USRS - 1,
                                                     (unsigned long) TL_USRS_UNKNOWN);

   /***** If parameter WhichUsrs is not present, get it from database *****/
   if (Gbl.Timeline.WhichUsrs == TL_USRS_UNKNOWN)
      Gbl.Timeline.WhichUsrs = TL_GetWhichUsrsFromDB ();

   /***** If parameter WhichUsrs is unknown, set it to default *****/
   if (Gbl.Timeline.WhichUsrs == TL_USRS_UNKNOWN)
      Gbl.Timeline.WhichUsrs = TL_DEFAULT_WHICH_USRS;
  }

/*****************************************************************************/
/********** Get user's last data from database giving a user's code **********/
/*****************************************************************************/

static TL_WhichUsrs_t TL_GetWhichUsrsFromDB (void)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned UnsignedNum;
   TL_WhichUsrs_t WhichUsrs = TL_USRS_UNKNOWN;

   /***** Get which users from database *****/
   if (DB_QuerySELECT (&mysql_res,"can not get timeline users from user's last data",
		       "SELECT TimelineUsrs"		   // row[0]
		       " FROM usr_last WHERE UsrCod=%ld",
		       Gbl.Usrs.Me.UsrDat.UsrCod) == 1)
     {
      row = mysql_fetch_row (mysql_res);

      /* Get which users */
      if (sscanf (row[0],"%u",&UnsignedNum) == 1)
         if (UnsignedNum < TL_NUM_WHICH_USRS)
            WhichUsrs = (TL_WhichUsrs_t) UnsignedNum;
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   return WhichUsrs;
  }

/*****************************************************************************/
/********************** Save which users into database ***********************/
/*****************************************************************************/

static void TL_SaveWhichUsersInDB (void)
  {
   if (Gbl.Usrs.Me.Logged)
     {
      if (Gbl.Timeline.WhichUsrs == TL_USRS_UNKNOWN)
	 Gbl.Timeline.WhichUsrs = TL_DEFAULT_WHICH_USRS;

      /***** Update which users in database *****/
      // WhichUsrs is stored in usr_last for next time I log in
      DB_QueryUPDATE ("can not update timeline users in user's last data",
		      "UPDATE usr_last SET TimelineUsrs=%u WHERE UsrCod=%ld",
		      (unsigned) Gbl.Timeline.WhichUsrs,
		      Gbl.Usrs.Me.UsrDat.UsrCod);
     }
  }

/*****************************************************************************/
/********* Get parameter with which users to view in global timeline *********/
/*****************************************************************************/

static void TL_ShowWarningYouDontFollowAnyUser (void)
  {
   extern const char *Txt_You_dont_follow_any_user;
   unsigned NumFollowing;
   unsigned NumFollowers;

   /***** Check if I follow someone *****/
   Fol_GetNumFollow (Gbl.Usrs.Me.UsrDat.UsrCod,&NumFollowing,&NumFollowers);
   if (!NumFollowing)
     {
      /***** Show warning if I do not follow anyone *****/
      Ale_ShowAlert (Ale_WARNING,Txt_You_dont_follow_any_user);

      /***** Put link to show users to follow *****/
      fprintf (Gbl.F.Out,"<div class=\"CONTEXT_MENU\">");
      Fol_PutLinkWhoToFollow ();
      fprintf (Gbl.F.Out,"</div>");
     }
  }

/*****************************************************************************/
/******************* Show new publications in timeline ***********************/
/*****************************************************************************/
// The publications are inserted as list elements of a hidden list

static void TL_InsertNewPubsInTimeline (char *Query)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned long NumPubsGot;
   unsigned long NumPub;
   struct TL_Publication SocPub;
   struct TL_Note SocNot;

   /***** Get new publications timeline from database *****/
   NumPubsGot = DB_QuerySELECT (&mysql_res,"can not get timeline",
				"%s",
				Query);

   /***** List new publications timeline *****/
   for (NumPub = 0;
	NumPub < NumPubsGot;
	NumPub++)
     {
      /* Get data of publication */
      row = mysql_fetch_row (mysql_res);
      TL_GetDataOfPublicationFromRow (row,&SocPub);

      /* Get data of note */
      SocNot.NotCod = SocPub.NotCod;
      TL_GetDataOfNoteByCod (&SocNot);

      /* Write note */
      TL_WriteNote (&SocNot,
                    SocPub.TopMessage,SocPub.PublisherCod,
                    false,false);
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/********************* Show old publications in timeline *********************/
/*****************************************************************************/
// The publications are inserted as list elements of a hidden list

static void TL_ShowOldPubsInTimeline (char *Query)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned long NumPubsGot;
   unsigned long NumPub;
   struct TL_Publication SocPub;
   struct TL_Note SocNot;

   /***** Get old publications timeline from database *****/
   NumPubsGot = DB_QuerySELECT (&mysql_res,"can not get timeline",
				"%s",
				Query);

   /***** List old publications in timeline *****/
   for (NumPub = 0, SocPub.PubCod = 0;
	NumPub < NumPubsGot;
	NumPub++)
     {
      /* Get data of publication */
      row = mysql_fetch_row (mysql_res);
      TL_GetDataOfPublicationFromRow (row,&SocPub);

      /* Get data of note */
      SocNot.NotCod = SocPub.NotCod;
      TL_GetDataOfNoteByCod (&SocNot);

      /* Write note */
      TL_WriteNote (&SocNot,
                    SocPub.TopMessage,SocPub.PublisherCod,
                    false,false);
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   /***** Store first publication code into session *****/
   TL_UpdateFirstPubCodIntoSession (SocPub.PubCod);
  }

/*****************************************************************************/
/***************** Put link to view new publications in timeline *************/
/*****************************************************************************/

static void TL_PutLinkToViewNewPublications (void)
  {
   extern const char *The_ClassFormInBoxBold[The_NUM_THEMES];
   extern const char *Txt_See_new_activity;

   /***** Link to view (show hidden) new publications *****/
   // div is hidden. When new posts arrive to the client via AJAX, div is shown
   fprintf (Gbl.F.Out,"<div id=\"view_new_posts_container\""
	              " class=\"TL_WIDTH TL_SEP VERY_LIGHT_BLUE\""
	              " style=\"display:none;\">"
                      "<a href=\"\" class=\"%s\""
                      " onclick=\"moveNewTimelineToTimeline(); return false;\" />"
                      "%s (<span id=\"view_new_posts_count\">0</span>)"
	              "</a>"
	              "</div>",
	    The_ClassFormInBoxBold[Gbl.Prefs.Theme],
	    Txt_See_new_activity);
  }

/*****************************************************************************/
/***************** Put link to view old publications in timeline *************/
/*****************************************************************************/

static void TL_PutLinkToViewOldPublications (void)
  {
   extern const char *The_ClassFormInBoxBold[The_NUM_THEMES];
   extern const char *Txt_See_more;

   /***** Animated link to view old publications *****/
   fprintf (Gbl.F.Out,"<div id=\"view_old_posts_container\""
	              " class=\"TL_WIDTH TL_SEP VERY_LIGHT_BLUE\">"
                      "<a href=\"\" class=\"%s\" onclick=\""
   		      "document.getElementById('get_old_timeline').style.display='none';"	// Icon to be hidden on click
		      "document.getElementById('getting_old_timeline').style.display='';"	// Icon to be shown on click
                      "refreshOldTimeline();"
		      "return false;\">"
	              "<img id=\"get_old_timeline\""
	              " src=\"%s/recycle16x16.gif\" alt=\"%s\" title=\"%s\""
		      " class=\"ICO20x20\" />"
		      "<img id=\"getting_old_timeline\""
		      " src=\"%s/working16x16.gif\" alt=\"%s\" title=\"%s\""
		      " class=\"ICO20x20\" style=\"display:none;\" />"				// Animated icon hidden
		      "&nbsp;%s"
	              "</a>"
	              "</div>",
	    The_ClassFormInBoxBold[Gbl.Prefs.Theme],
	    Gbl.Prefs.URLIcons,Txt_See_more,Txt_See_more,
	    Gbl.Prefs.URLIcons,Txt_See_more,Txt_See_more,
	    Txt_See_more);
  }

/*****************************************************************************/
/******************************** Write note *********************************/
/*****************************************************************************/
// All forms in this function and nested functions must have unique identifiers

static void TL_WriteNote (const struct TL_Note *SocNot,
                          TL_TopMessage_t TopMessage,long UsrCod,
                          bool Highlight,	// Highlight note
                          bool ShowNoteAlone)	// Note is shown alone, not in a list
  {
   extern const char *Txt_Forum;
   extern const char *Txt_Course;
   extern const char *Txt_Degree;
   extern const char *Txt_Centre;
   extern const char *Txt_Institution;
   struct UsrData UsrDat;
   bool ItsMe;
   bool IAmTheAuthor = false;
   bool IAmASharerOfThisSocNot = false;
   bool IAmAFaverOfThisSocNot = false;
   struct Instit Ins;
   struct Centre Ctr;
   struct Degree Deg;
   struct Course Crs;
   bool ShowPhoto = false;
   char PhotoURL[PATH_MAX + 1];
   char ForumName[For_MAX_BYTES_FORUM_NAME + 1];
   char SummaryStr[Ntf_MAX_BYTES_SUMMARY + 1];
   unsigned NumComments;
   char IdNewComment[Frm_MAX_BYTES_ID + 1];
   static unsigned NumDiv = 0;	// Used to create unique div id for fav and shared

   NumDiv++;

   /***** Start box ****/
   if (ShowNoteAlone)
     {
      Box_StartBox (NULL,NULL,NULL,
                    NULL,Box_CLOSABLE);
      fprintf (Gbl.F.Out,"<ul class=\"TL_LIST\">");
     }

   /***** Start list item *****/
   fprintf (Gbl.F.Out,"<li class=\"TL_WIDTH");
   if (!ShowNoteAlone)
      fprintf (Gbl.F.Out," TL_SEP");
   if (Highlight)
      fprintf (Gbl.F.Out," TL_NEW_PUB");
   fprintf (Gbl.F.Out,"\">");

   if (SocNot->NotCod   <= 0 ||
       SocNot->NoteType == TL_NOTE_UNKNOWN ||
       SocNot->UsrCod   <= 0)
      Ale_ShowAlert (Ale_ERROR,"Error in note.");
   else
     {
      /***** Initialize location in hierarchy *****/
      Ins.InsCod = -1L;
      Ctr.CtrCod = -1L;
      Deg.DegCod = -1L;
      Crs.CrsCod = -1L;

      /***** Write sharer/commenter if distinct to author *****/
      TL_WriteTopMessage (TopMessage,UsrCod);

      /***** Initialize structure with user's data *****/
      Usr_UsrDataConstructor (&UsrDat);

      /***** Get author data *****/
      UsrDat.UsrCod = SocNot->UsrCod;
      Usr_ChkUsrCodAndGetAllUsrDataFromUsrCod (&UsrDat);
      if (Gbl.Usrs.Me.Logged)
	{
	 ItsMe = Usr_ItsMe (UsrDat.UsrCod);
	 IAmTheAuthor = ItsMe;
	 if (!IAmTheAuthor)
	   {
	    IAmASharerOfThisSocNot = TL_CheckIfNoteIsSharedByUsr (SocNot->NotCod,
								   Gbl.Usrs.Me.UsrDat.UsrCod);
	    IAmAFaverOfThisSocNot  = TL_CheckIfNoteIsFavedByUsr  (SocNot->NotCod,
								   Gbl.Usrs.Me.UsrDat.UsrCod);
	   }
	}

      /***** Left: write author's photo *****/
      fprintf (Gbl.F.Out,"<div class=\"TL_LEFT_PHOTO\">");
      ShowPhoto = Pho_ShowingUsrPhotoIsAllowed (&UsrDat,PhotoURL);
      Pho_ShowUsrPhoto (&UsrDat,ShowPhoto ? PhotoURL :
					    NULL,
			"PHOTO42x56",Pho_ZOOM,true);	// Use unique id
      fprintf (Gbl.F.Out,"</div>");

      /***** Right: author's name, time, summary and buttons *****/
      fprintf (Gbl.F.Out,"<div class=\"TL_RIGHT_CONTAINER TL_RIGHT_WIDTH\">");

      /* Write author's full name and nickname */
      TL_WriteAuthorNote (&UsrDat);

      /* Write date and time */
      TL_WriteDateTime (SocNot->DateTimeUTC);

      /* Write content of the note */
      if (SocNot->NoteType == TL_NOTE_POST)
	 /* Write post content */
	 TL_GetAndWritePost (SocNot->Cod);
      else
	{
	 /* Get location in hierarchy */
	 if (!SocNot->Unavailable)
	    switch (SocNot->NoteType)
	      {
	       case TL_NOTE_INS_DOC_PUB_FILE:
	       case TL_NOTE_INS_SHA_PUB_FILE:
		  /* Get institution data */
		  Ins.InsCod = SocNot->HieCod;
		  Ins_GetDataOfInstitutionByCod (&Ins,Ins_GET_BASIC_DATA);
		  break;
	       case TL_NOTE_CTR_DOC_PUB_FILE:
	       case TL_NOTE_CTR_SHA_PUB_FILE:
		  /* Get centre data */
		  Ctr.CtrCod = SocNot->HieCod;
		  Ctr_GetDataOfCentreByCod (&Ctr);
		  break;
	       case TL_NOTE_DEG_DOC_PUB_FILE:
	       case TL_NOTE_DEG_SHA_PUB_FILE:
		  /* Get degree data */
		  Deg.DegCod = SocNot->HieCod;
		  Deg_GetDataOfDegreeByCod (&Deg);
		  break;
	       case TL_NOTE_CRS_DOC_PUB_FILE:
	       case TL_NOTE_CRS_SHA_PUB_FILE:
	       case TL_NOTE_EXAM_ANNOUNCEMENT:
	       case TL_NOTE_NOTICE:
		  /* Get course data */
		  Crs.CrsCod = SocNot->HieCod;
		  Crs_GetDataOfCourseByCod (&Crs);
		  break;
	       case TL_NOTE_FORUM_POST:
		  /* Get forum type of the post */
		  For_GetForumTypeAndLocationOfAPost (SocNot->Cod,&Gbl.Forum.ForumSelected);
		  For_SetForumName (&Gbl.Forum.ForumSelected,
		                    ForumName,Gbl.Prefs.Language,false);	// Set forum name in recipient's language
		  break;
	       default:
		  break;
	      }

	 /* Write note type */
	 TL_PutFormGoToAction (SocNot);

	 /* Write location in hierarchy */
	 if (!SocNot->Unavailable)
	    switch (SocNot->NoteType)
	      {
	       case TL_NOTE_INS_DOC_PUB_FILE:
	       case TL_NOTE_INS_SHA_PUB_FILE:
		  /* Write location (institution) in hierarchy */
		  fprintf (Gbl.F.Out,"<div class=\"DAT\">%s: %s</div>",
			   Txt_Institution,Ins.ShrtName);
		  break;
	       case TL_NOTE_CTR_DOC_PUB_FILE:
	       case TL_NOTE_CTR_SHA_PUB_FILE:
		  /* Write location (centre) in hierarchy */
		  fprintf (Gbl.F.Out,"<div class=\"DAT\">%s: %s</div>",
			   Txt_Centre,Ctr.ShrtName);
		  break;
	       case TL_NOTE_DEG_DOC_PUB_FILE:
	       case TL_NOTE_DEG_SHA_PUB_FILE:
		  /* Write location (degree) in hierarchy */
		  fprintf (Gbl.F.Out,"<div class=\"DAT\">%s: %s</div>",
			   Txt_Degree,Deg.ShrtName);
		  break;
	       case TL_NOTE_CRS_DOC_PUB_FILE:
	       case TL_NOTE_CRS_SHA_PUB_FILE:
	       case TL_NOTE_EXAM_ANNOUNCEMENT:
	       case TL_NOTE_NOTICE:
		  /* Write location (course) in hierarchy */
		  fprintf (Gbl.F.Out,"<div class=\"DAT\">%s: %s</div>",
			   Txt_Course,Crs.ShrtName);
		  break;
	       case TL_NOTE_FORUM_POST:
		  /* Write forum name */
		  fprintf (Gbl.F.Out,"<div class=\"DAT\">%s: %s</div>",
			   Txt_Forum,ForumName);
		  break;
	       default:
		  break;
	      }

	 /* Write note summary */
	 TL_GetNoteSummary (SocNot,SummaryStr);
	 fprintf (Gbl.F.Out,"<div class=\"DAT\">%s</div>",SummaryStr);
	}

      /* End of right part */
      fprintf (Gbl.F.Out,"</div>");

      fprintf (Gbl.F.Out,"<div class=\"TL_BOTTOM_LEFT\">");

      /* Create unique id for new comment */
      Frm_SetUniqueId (IdNewComment);

      /* Get number of comments in this note */
      NumComments = TL_GetNumCommentsInNote (SocNot->NotCod);

      /* Put icon to add a comment */
      if (SocNot->Unavailable)		// Unavailable notes can not be commented
	 TL_PutIconCommentDisabled ();
      else
         TL_PutIconToToggleCommentNote (IdNewComment);

      fprintf (Gbl.F.Out,"</div>");

      fprintf (Gbl.F.Out,"<div class=\"TL_BOTTOM_RIGHT TL_RIGHT_WIDTH\">"
	                 "<div class=\"TL_ICO_FAV_SHA_REM\">");

      /* Put icon to mark this note as favourite */
      if (SocNot->Unavailable ||	// Unavailable notes can not be favourited
          IAmTheAuthor)			// I am the author
        {
	 /* Put disabled icon and list of users
	    who have marked this note as favourite */
	 TL_PutDisabledIconFav (SocNot->NumFavs);
         TL_ShowUsrsWhoHaveMarkedNoteAsFav (SocNot);
        }
      else				// Available and I am not the author
	{
	 fprintf (Gbl.F.Out,"<div id=\"fav_not_%s_%u\""
	                    " class=\"TL_ICO_FAV\">",
	          Gbl.UniqueNameEncrypted,NumDiv);
	 if (IAmAFaverOfThisSocNot)	// I have favourited this note
	    /* Put icon to unfav this publication and list of users */
	    TL_PutFormToUnfavNote (SocNot);
	 else				//  I am not a faver of this note
	    /* Put icon to fav this publication and list of users */
	    TL_PutFormToFavNote (SocNot);
	 fprintf (Gbl.F.Out,"</div>");
	}

      /* Put icon to share/unshare */
      if (SocNot->Unavailable ||	// Unavailable notes can not be shared
          IAmTheAuthor)			// I am the author
        {
	 /* Put disabled icon and list of users
	    who have shared this note */
	 TL_PutDisabledIconShare (SocNot->NumShared);
         TL_ShowUsrsWhoHaveSharedNote (SocNot);
        }
      else				// Available and I am not the author
        {
	 fprintf (Gbl.F.Out,"<div id=\"sha_not_%s_%u\""
			    " class=\"TL_ICO_SHA\">",
		  Gbl.UniqueNameEncrypted,NumDiv);
	 if (IAmASharerOfThisSocNot)	// I am a sharer of this note
	    /* Put icon to unshare this publication and list of users */
	    TL_PutFormToUnshareNote (SocNot);
	 else				// I am not a sharer of this note
	    /* Put icon to share this publication and list of users */
	    TL_PutFormToShareNote (SocNot);
	 fprintf (Gbl.F.Out,"</div>");
        }

      /* Put icon to remove this note */
      if (IAmTheAuthor)
	 TL_PutFormToRemovePublication (SocNot->NotCod);

      /* End of icon bar */
      fprintf (Gbl.F.Out,"</div>");

      /* Show comments */
      if (NumComments)
	 TL_WriteCommentsInNote (SocNot);

      /* End of bottom right */
      fprintf (Gbl.F.Out,"</div>");

      /* Put hidden form to write a new comment */
      TL_PutHiddenFormToWriteNewCommentToNote (SocNot->NotCod,IdNewComment);

      /***** Free memory used for user's data *****/
      Usr_UsrDataDestructor (&UsrDat);
     }

   /***** End list item *****/
   fprintf (Gbl.F.Out,"</li>");

   /***** End box ****/
   if (ShowNoteAlone)
     {
      fprintf (Gbl.F.Out,"</ul>");
      Box_EndBox ();
     }
  }

/*****************************************************************************/
/*************** Write sharer/commenter if distinct to author ****************/
/*****************************************************************************/
// All forms in this function and nested functions must have unique identifiers

static void TL_WriteTopMessage (TL_TopMessage_t TopMessage,long UsrCod)
  {
   extern const char *Txt_My_public_profile;
   extern const char *Txt_Another_user_s_profile;
   extern const char *Txt_TIMELINE_NOTE_TOP_MESSAGES[TL_NUM_TOP_MESSAGES];
   struct UsrData UsrDat;
   bool ItsMe = Usr_ItsMe (UsrCod);

   if (TopMessage != TL_TOP_MESSAGE_NONE)
     {
      /***** Initialize structure with user's data *****/
      Usr_UsrDataConstructor (&UsrDat);

      /***** Get user's data *****/
      UsrDat.UsrCod = UsrCod;
      if (Usr_ChkUsrCodAndGetAllUsrDataFromUsrCod (&UsrDat))	// Really we only need EncryptedUsrCod and FullName
	{
	 fprintf (Gbl.F.Out,"<div class=\"TL_TOP_CONTAINER"
	                    " TL_TOP_PUBLISHER TL_WIDTH\">");

	 /***** Show user's name inside form to go to user's public profile *****/
	 Frm_StartFormUnique (ActSeeOthPubPrf);
	 Usr_PutParamUsrCodEncrypted (UsrDat.EncryptedUsrCod);
	 Frm_LinkFormSubmitUnique (ItsMe ? Txt_My_public_profile :
					   Txt_Another_user_s_profile,
				   "TL_TOP_PUBLISHER");
	 fprintf (Gbl.F.Out,"%s</a>",UsrDat.FullName);
	 Frm_EndForm ();

	 /***** Show action made *****/
         fprintf (Gbl.F.Out," %s:</div>",
                  Txt_TIMELINE_NOTE_TOP_MESSAGES[TopMessage]);
	}

      /***** Free memory used for user's data *****/
      Usr_UsrDataDestructor (&UsrDat);
     }
  }

/*****************************************************************************/
/*************** Write name and nickname of author of a note *****************/
/*****************************************************************************/
// All forms in this function and nested functions must have unique identifiers

static void TL_WriteAuthorNote (const struct UsrData *UsrDat)
  {
   extern const char *Txt_My_public_profile;
   extern const char *Txt_Another_user_s_profile;
   bool ItsMe = Usr_ItsMe (UsrDat->UsrCod);

   fprintf (Gbl.F.Out,"<div class=\"TL_RIGHT_AUTHOR TL_RIGHT_AUTHOR_WIDTH\">");

   /***** Show user's name inside form to go to user's public profile *****/
   Frm_StartFormUnique (ActSeeOthPubPrf);
   Usr_PutParamUsrCodEncrypted (UsrDat->EncryptedUsrCod);
   Frm_LinkFormSubmitUnique (ItsMe ? Txt_My_public_profile :
				     Txt_Another_user_s_profile,
			     "DAT_N_BOLD");
   fprintf (Gbl.F.Out,"%s</a>",UsrDat->FullName);
   Frm_EndForm ();

   /***** Show user's nickname inside form to go to user's public profile *****/
   Frm_StartFormUnique (ActSeeOthPubPrf);
   Usr_PutParamUsrCodEncrypted (UsrDat->EncryptedUsrCod);
   Frm_LinkFormSubmitUnique (ItsMe ? Txt_My_public_profile :
				     Txt_Another_user_s_profile,
			     "DAT_LIGHT");
   fprintf (Gbl.F.Out," @%s</a>",UsrDat->Nickname);
   Frm_EndForm ();

   fprintf (Gbl.F.Out,"</div>");
  }

/*****************************************************************************/
/******************* Write the date of creation of a note ********************/
/*****************************************************************************/
// TimeUTC holds UTC date and time in UNIX format (seconds since 1970)

static void TL_WriteDateTime (time_t TimeUTC)
  {
   extern const char *Txt_Today;
   char IdDateTime[Frm_MAX_BYTES_ID + 1];

   /***** Create unique Id *****/
   Frm_SetUniqueId (IdDateTime);

   /***** Container where the date-time is written *****/
   fprintf (Gbl.F.Out,"<div id=\"%s\" class=\"TL_RIGHT_TIME DAT_LIGHT\">"
	              "</div>",
            IdDateTime);

   /***** Script to write date and time in browser local time *****/
   // This must be out of the div where the output is written
   // because it will be evaluated in a loop in JavaScript
   fprintf (Gbl.F.Out,"<script type=\"text/javascript\">"
		      "writeLocalDateHMSFromUTC('%s',%ld,"
		      "%u,',&nbsp;','%s',true,false,0x6);"
                      "</script>",
            IdDateTime,(long) TimeUTC,
            (unsigned) Gbl.Prefs.DateFormat,Txt_Today);
  }

/*****************************************************************************/
/***************** Get from database and write public post *******************/
/*****************************************************************************/

static void TL_GetAndWritePost (long PstCod)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned long NumRows;
   char Content[Cns_MAX_BYTES_LONG_TEXT + 1];
   struct Media Media;

   /***** Initialize image *****/
   Med_MediaConstructor (&Media);

   /***** Get post from database *****/
   NumRows = DB_QuerySELECT (&mysql_res,"can not get the content"
					" of a post",
			     "SELECT Content,"		// row[0]
			            "MediaName,"	// row[1]
			            "MediaType,"	// row[2]
			            "MediaTitle,"	// row[3]
			            "MediaURL"		// row[4]
			     " FROM social_posts"
			     " WHERE PstCod=%ld",
			     PstCod);

   /***** Result should have a unique row *****/
   if (NumRows == 1)
     {
      row = mysql_fetch_row (mysql_res);

      /****** Get content (row[0]) *****/
      Str_Copy (Content,row[0],
                Cns_MAX_BYTES_LONG_TEXT);

      /****** Get media data (row[1], row[2], row[3], row[4]) *****/
      Med_GetMediaDataFromRow (row[1],row[2],row[3],row[4],&Media);
     }
   else
      Content[0] = '\0';

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   /***** Write content *****/
   if (Content[0])
     {
      fprintf (Gbl.F.Out,"<div class=\"TL_TXT\">");
      Msg_WriteMsgContent (Content,Cns_MAX_BYTES_LONG_TEXT,true,false);
      fprintf (Gbl.F.Out,"</div>");
     }

   /***** Show image *****/
   Med_ShowMedia (&Media,"TL_POST_IMG_CONTAINER TL_RIGHT_WIDTH",
	                 "TL_POST_IMG TL_RIGHT_WIDTH");

   /***** Free image *****/
   Med_MediaDestructor (&Media);
  }

/*****************************************************************************/
/************* Put form to go to an action depending on the note *************/
/*****************************************************************************/
// All forms in this function and nested functions must have unique identifiers

static void TL_PutFormGoToAction (const struct TL_Note *SocNot)
  {
   extern const Act_Action_t For_ActionsSeeFor[For_NUM_TYPES_FORUM];
   extern const char *The_ClassFormOutBoxBold[The_NUM_THEMES];
   extern const char *Txt_TIMELINE_NOTE[TL_NUM_NOTE_TYPES];
   extern const char *Txt_not_available;
   char Class[64];
   const Act_Action_t TL_DefaultActions[TL_NUM_NOTE_TYPES] =
     {
      ActUnk,			// TL_NOTE_UNKNOWN

      /* Institution tab */
      ActReqDatSeeDocIns,	// TL_NOTE_INS_DOC_PUB_FILE
      ActReqDatShaIns,		// TL_NOTE_INS_SHA_PUB_FILE

      /* Centre tab */
      ActReqDatSeeDocCtr,	// TL_NOTE_CTR_DOC_PUB_FILE
      ActReqDatShaCtr,		// TL_NOTE_CTR_SHA_PUB_FILE

      /* Degree tab */
      ActReqDatSeeDocDeg,	// TL_NOTE_DEG_DOC_PUB_FILE
      ActReqDatShaDeg,		// TL_NOTE_DEG_SHA_PUB_FILE

      /* Course tab */
      ActReqDatSeeDocCrs,	// TL_NOTE_CRS_DOC_PUB_FILE
      ActReqDatShaCrs,		// TL_NOTE_CRS_SHA_PUB_FILE

      /* Assessment tab */
      ActSeeOneExaAnn,		// TL_NOTE_EXAM_ANNOUNCEMENT

      /* Users tab */

      /* Start tab */
      ActUnk,			// TL_NOTE_SOCIAL_POST (action not used)	// TODO: Move to start tab
      ActSeeFor,		// TL_NOTE_FORUM_POST				// TODO: Move to messages tab

      /* Messages tab */
      ActSeeOneNot,		// TL_NOTE_NOTICE

      /* Statistics tab */

      /* Profile tab */

     };
   const char *TL_Icons[TL_NUM_NOTE_TYPES] =
     {
      NULL,			// TL_NOTE_UNKNOWN

      /* Institution tab */
      "file.svg",		// TL_NOTE_INS_DOC_PUB_FILE
      "file.svg",		// TL_NOTE_INS_SHA_PUB_FILE

      /* Centre tab */
      "file.svg",		// TL_NOTE_CTR_DOC_PUB_FILE
      "file.svg",		// TL_NOTE_CTR_SHA_PUB_FILE

      /* Degree tab */
      "file.svg",		// TL_NOTE_DEG_DOC_PUB_FILE
      "file.svg",		// TL_NOTE_DEG_SHA_PUB_FILE

      /* Course tab */
      "file.svg",		// TL_NOTE_CRS_DOC_PUB_FILE
      "file.svg",		// TL_NOTE_CRS_SHA_PUB_FILE

      /* Assessment tab */
      "bullhorn.svg",		// TL_NOTE_EXAM_ANNOUNCEMENT

      /* Users tab */

      /* Start tab */
      NULL,			// TL_NOTE_SOCIAL_POST (icon not used)

      /* Messages tab */
      "comments.svg",		// TL_NOTE_FORUM_POST	// TODO: Move down
      "sticky-note.svg",	// TL_NOTE_NOTICE

      /* Statistics tab */

      /* Profile tab */

     };

   if (SocNot->Unavailable ||	// File/notice... pointed by this note is unavailable
       Gbl.Form.Inside)		// Inside another form
     {
      /***** Do not put form *****/
      fprintf (Gbl.F.Out,"<div class=\"DAT_LIGHT\">%s",
               Txt_TIMELINE_NOTE[SocNot->NoteType]);
      if (SocNot->Unavailable)
         fprintf (Gbl.F.Out," (%s)",Txt_not_available);
      fprintf (Gbl.F.Out,"</div>");
     }
   else			// Not inside another form
     {
      fprintf (Gbl.F.Out,"<div>");

      /***** Parameters depending on the type of note *****/
      switch (SocNot->NoteType)
	{
	 case TL_NOTE_INS_DOC_PUB_FILE:
	 case TL_NOTE_INS_SHA_PUB_FILE:
	    Frm_StartFormUnique (TL_DefaultActions[SocNot->NoteType]);
	    Brw_PutHiddenParamFilCod (SocNot->Cod);
	    if (SocNot->HieCod != Gbl.CurrentIns.Ins.InsCod)	// Not the current institution
	       Ins_PutParamInsCod (SocNot->HieCod);		// Go to another institution
	    break;
	 case TL_NOTE_CTR_DOC_PUB_FILE:
	 case TL_NOTE_CTR_SHA_PUB_FILE:
	    Frm_StartFormUnique (TL_DefaultActions[SocNot->NoteType]);
	    Brw_PutHiddenParamFilCod (SocNot->Cod);
	    if (SocNot->HieCod != Gbl.CurrentCtr.Ctr.CtrCod)	// Not the current centre
	       Ctr_PutParamCtrCod (SocNot->HieCod);		// Go to another centre
	    break;
	 case TL_NOTE_DEG_DOC_PUB_FILE:
	 case TL_NOTE_DEG_SHA_PUB_FILE:
	    Frm_StartFormUnique (TL_DefaultActions[SocNot->NoteType]);
	    Brw_PutHiddenParamFilCod (SocNot->Cod);
	    if (SocNot->HieCod != Gbl.CurrentDeg.Deg.DegCod)	// Not the current degree
	       Deg_PutParamDegCod (SocNot->HieCod);		// Go to another degree
	    break;
	 case TL_NOTE_CRS_DOC_PUB_FILE:
	 case TL_NOTE_CRS_SHA_PUB_FILE:
	    Frm_StartFormUnique (TL_DefaultActions[SocNot->NoteType]);
	    Brw_PutHiddenParamFilCod (SocNot->Cod);
	    if (SocNot->HieCod != Gbl.CurrentCrs.Crs.CrsCod)	// Not the current course
	       Crs_PutParamCrsCod (SocNot->HieCod);		// Go to another course
	    break;
	 case TL_NOTE_EXAM_ANNOUNCEMENT:
	    Frm_StartFormUnique (TL_DefaultActions[SocNot->NoteType]);
	    Exa_PutHiddenParamExaCod (SocNot->Cod);
	    if (SocNot->HieCod != Gbl.CurrentCrs.Crs.CrsCod)	// Not the current course
	       Crs_PutParamCrsCod (SocNot->HieCod);		// Go to another course
	    break;
	 case TL_NOTE_POST:	// Not applicable
	    return;
	 case TL_NOTE_FORUM_POST:
	    Frm_StartFormUnique (For_ActionsSeeFor[Gbl.Forum.ForumSelected.Type]);
	    For_PutAllHiddenParamsForum (1,	// Page of threads = first
                                         1,	// Page of posts   = first
                                         Gbl.Forum.ForumSet,
					 Gbl.Forum.ThreadsOrder,
					 Gbl.Forum.ForumSelected.Location,
					 Gbl.Forum.ForumSelected.ThrCod,
					 -1L);
	    if (SocNot->HieCod != Gbl.CurrentCrs.Crs.CrsCod)	// Not the current course
	       Crs_PutParamCrsCod (SocNot->HieCod);		// Go to another course
	    break;
	 case TL_NOTE_NOTICE:
	    Frm_StartFormUnique (TL_DefaultActions[SocNot->NoteType]);
	    Not_PutHiddenParamNotCod (SocNot->Cod);
	    if (SocNot->HieCod != Gbl.CurrentCrs.Crs.CrsCod)	// Not the current course
	       Crs_PutParamCrsCod (SocNot->HieCod);		// Go to another course
	    break;
	 default:			// Not applicable
	    return;
	}

      /***** Link and end form *****/
      snprintf (Class,sizeof (Class),
	        "%s ICO_HIGHLIGHT",
		The_ClassFormOutBoxBold[Gbl.Prefs.Theme]);
      Frm_LinkFormSubmitUnique (Txt_TIMELINE_NOTE[SocNot->NoteType],Class);
      fprintf (Gbl.F.Out,"<img src=\"%s/%s\""
	                 " alt=\"%s\" title=\"%s\""
	                 " class=\"CONTEXT_ICO_x16\" />"
	                 "&nbsp;%s"
	                 "</a>",
            Gbl.Prefs.URLIcons,TL_Icons[SocNot->NoteType],
            Txt_TIMELINE_NOTE[SocNot->NoteType],
            Txt_TIMELINE_NOTE[SocNot->NoteType],
            Txt_TIMELINE_NOTE[SocNot->NoteType]);
      Frm_EndForm ();

      fprintf (Gbl.F.Out,"</div>");
     }
  }

/*****************************************************************************/
/********************** Get note summary and content *************************/
/*****************************************************************************/

static void TL_GetNoteSummary (const struct TL_Note *SocNot,
                               char SummaryStr[Ntf_MAX_BYTES_SUMMARY + 1])
  {
   SummaryStr[0] = '\0';

   switch (SocNot->NoteType)
     {
      case TL_NOTE_UNKNOWN:
          break;
      case TL_NOTE_INS_DOC_PUB_FILE:
      case TL_NOTE_INS_SHA_PUB_FILE:
      case TL_NOTE_CTR_DOC_PUB_FILE:
      case TL_NOTE_CTR_SHA_PUB_FILE:
      case TL_NOTE_DEG_DOC_PUB_FILE:
      case TL_NOTE_DEG_SHA_PUB_FILE:
      case TL_NOTE_CRS_DOC_PUB_FILE:
      case TL_NOTE_CRS_SHA_PUB_FILE:
	 Brw_GetSummaryAndContentOfFile (SummaryStr,NULL,SocNot->Cod,false);
         break;
      case TL_NOTE_EXAM_ANNOUNCEMENT:
         Exa_GetSummaryAndContentExamAnnouncement (SummaryStr,NULL,SocNot->Cod,false);
         break;
      case TL_NOTE_POST:
	 // Not applicable
         break;
      case TL_NOTE_FORUM_POST:
         For_GetSummaryAndContentForumPst (SummaryStr,NULL,SocNot->Cod,false);
         break;
      case TL_NOTE_NOTICE:
         Not_GetSummaryAndContentNotice (SummaryStr,NULL,SocNot->Cod,false);
         break;
     }
  }

/*****************************************************************************/
/***************** Store and publish a note into database ********************/
/*****************************************************************************/
// Return the code of the new note just created

void TL_StoreAndPublishNote (TL_NoteType_t NoteType,long Cod,struct TL_Publication *SocPub)
  {
   long HieCod;	// Hierarchy code (institution/centre/degree/course)

   switch (NoteType)
     {
      case TL_NOTE_INS_DOC_PUB_FILE:
      case TL_NOTE_INS_SHA_PUB_FILE:
	 HieCod = Gbl.CurrentIns.Ins.InsCod;
	 break;
      case TL_NOTE_CTR_DOC_PUB_FILE:
      case TL_NOTE_CTR_SHA_PUB_FILE:
	 HieCod = Gbl.CurrentCtr.Ctr.CtrCod;
	 break;
      case TL_NOTE_DEG_DOC_PUB_FILE:
      case TL_NOTE_DEG_SHA_PUB_FILE:
	 HieCod = Gbl.CurrentDeg.Deg.DegCod;
	 break;
      case TL_NOTE_CRS_DOC_PUB_FILE:
      case TL_NOTE_CRS_SHA_PUB_FILE:
      case TL_NOTE_EXAM_ANNOUNCEMENT:
      case TL_NOTE_NOTICE:
	 HieCod = Gbl.CurrentCrs.Crs.CrsCod;
	 break;
      default:
	 HieCod = -1L;
         break;
     }

   /***** Store note *****/
   SocPub->NotCod =
   DB_QueryINSERTandReturnCode ("can not create new note",
				"INSERT INTO social_notes"
				" (NoteType,Cod,UsrCod,HieCod,Unavailable,TimeNote)"
				" VALUES"
				" (%u,%ld,%ld,%ld,'N',NOW())",
				(unsigned) NoteType,
				Cod,Gbl.Usrs.Me.UsrDat.UsrCod,HieCod);

   /***** Publish note in timeline *****/
   SocPub->PublisherCod = Gbl.Usrs.Me.UsrDat.UsrCod;
   SocPub->PubType      = TL_PUB_ORIGINAL_NOTE;
   TL_PublishNoteInTimeline (SocPub);
  }

/*****************************************************************************/
/************************* Mark a note as unavailable ************************/
/*****************************************************************************/

void TL_MarkNoteAsUnavailableUsingNotCod (long NotCod)
  {
   /***** Mark the note as unavailable *****/
   DB_QueryUPDATE ("can not mark note as unavailable",
		   "UPDATE social_notes SET Unavailable='Y'"
		   " WHERE NotCod=%ld",
		   NotCod);
  }

void TL_MarkNoteAsUnavailableUsingNoteTypeAndCod (TL_NoteType_t NoteType,long Cod)
  {
   /***** Mark the note as unavailable *****/
   DB_QueryUPDATE ("can not mark note as unavailable",
		   "UPDATE social_notes SET Unavailable='Y'"
		   " WHERE NoteType=%u AND Cod=%ld",
		   (unsigned) NoteType,Cod);
  }

/*****************************************************************************/
/****************** Mark notes of one file as unavailable ********************/
/*****************************************************************************/

void TL_MarkNoteOneFileAsUnavailable (const char *Path)
  {
   extern const Brw_FileBrowser_t Brw_FileBrowserForDB_files[Brw_NUM_TYPES_FILE_BROWSER];
   Brw_FileBrowser_t FileBrowser = Brw_FileBrowserForDB_files[Gbl.FileBrowser.Type];
   long FilCod;
   TL_NoteType_t NoteType;

   switch (FileBrowser)
     {
      case Brw_ADMI_DOC_INS:
      case Brw_ADMI_SHR_INS:
      case Brw_ADMI_DOC_CTR:
      case Brw_ADMI_SHR_CTR:
      case Brw_ADMI_DOC_DEG:
      case Brw_ADMI_SHR_DEG:
      case Brw_ADMI_DOC_CRS:
      case Brw_ADMI_SHR_CRS:
         /***** Get file code *****/
	 FilCod = Brw_GetFilCodByPath (Path,true);	// Only if file is public
	 if (FilCod > 0)
	   {
	    /***** Mark possible note as unavailable *****/
	    switch (FileBrowser)
	      {
	       case Brw_ADMI_DOC_INS:
		  NoteType = TL_NOTE_INS_DOC_PUB_FILE;
		  break;
	       case Brw_ADMI_SHR_INS:
		  NoteType = TL_NOTE_INS_SHA_PUB_FILE;
		  break;
	       case Brw_ADMI_DOC_CTR:
		  NoteType = TL_NOTE_CTR_DOC_PUB_FILE;
		  break;
	       case Brw_ADMI_SHR_CTR:
		  NoteType = TL_NOTE_CTR_SHA_PUB_FILE;
		  break;
	       case Brw_ADMI_DOC_DEG:
		  NoteType = TL_NOTE_DEG_DOC_PUB_FILE;
		  break;
	       case Brw_ADMI_SHR_DEG:
		  NoteType = TL_NOTE_DEG_SHA_PUB_FILE;
		  break;
	       case Brw_ADMI_DOC_CRS:
		  NoteType = TL_NOTE_CRS_DOC_PUB_FILE;
		  break;
	       case Brw_ADMI_SHR_CRS:
		  NoteType = TL_NOTE_CRS_SHA_PUB_FILE;
		  break;
	       default:
		  return;
	      }
	    TL_MarkNoteAsUnavailableUsingNoteTypeAndCod (NoteType,FilCod);
	   }
         break;
      default:
	 break;
     }
  }

/*****************************************************************************/
/***** Mark possible notes involving children of a folder as unavailable *****/
/*****************************************************************************/

void TL_MarkNotesChildrenOfFolderAsUnavailable (const char *Path)
  {
   extern const Brw_FileBrowser_t Brw_FileBrowserForDB_files[Brw_NUM_TYPES_FILE_BROWSER];
   Brw_FileBrowser_t FileBrowser = Brw_FileBrowserForDB_files[Gbl.FileBrowser.Type];
   long Cod = Brw_GetCodForFiles ();
   TL_NoteType_t NoteType;

   switch (FileBrowser)
     {
      case Brw_ADMI_DOC_INS:
      case Brw_ADMI_SHR_INS:
      case Brw_ADMI_DOC_CTR:
      case Brw_ADMI_SHR_CTR:
      case Brw_ADMI_DOC_DEG:
      case Brw_ADMI_SHR_DEG:
      case Brw_ADMI_DOC_CRS:
      case Brw_ADMI_SHR_CRS:
	 /***** Mark possible note as unavailable *****/
	 switch (FileBrowser)
	   {
	    case Brw_ADMI_DOC_INS:
	       NoteType = TL_NOTE_INS_DOC_PUB_FILE;
	       break;
	    case Brw_ADMI_SHR_INS:
	       NoteType = TL_NOTE_INS_SHA_PUB_FILE;
	       break;
	    case Brw_ADMI_DOC_CTR:
	       NoteType = TL_NOTE_CTR_DOC_PUB_FILE;
	       break;
	    case Brw_ADMI_SHR_CTR:
	       NoteType = TL_NOTE_CTR_SHA_PUB_FILE;
	       break;
	    case Brw_ADMI_DOC_DEG:
	       NoteType = TL_NOTE_DEG_DOC_PUB_FILE;
	       break;
	    case Brw_ADMI_SHR_DEG:
	       NoteType = TL_NOTE_DEG_SHA_PUB_FILE;
	       break;
	    case Brw_ADMI_DOC_CRS:
	       NoteType = TL_NOTE_CRS_DOC_PUB_FILE;
	       break;
	    case Brw_ADMI_SHR_CRS:
	       NoteType = TL_NOTE_CRS_SHA_PUB_FILE;
	       break;
	    default:
	       return;
	   }
         DB_QueryUPDATE ("can not mark notes as unavailable",
			 "UPDATE social_notes SET Unavailable='Y'"
		         " WHERE NoteType=%u AND Cod IN"
	                 " (SELECT FilCod FROM files"
			 " WHERE FileBrowser=%u AND Cod=%ld"
			 " AND Path LIKE '%s/%%' AND Public='Y')",	// Only public files
			 (unsigned) NoteType,
			 (unsigned) FileBrowser,Cod,
			 Path);
         break;
      default:
	 break;
     }
  }

/*****************************************************************************/
/************************* Publish note in timeline **************************/
/*****************************************************************************/
// SocPub->PubCod is set by the function

static void TL_PublishNoteInTimeline (struct TL_Publication *SocPub)
  {
   /***** Publish note in timeline *****/
   SocPub->PubCod =
   DB_QueryINSERTandReturnCode ("can not publish note",
				"INSERT INTO social_pubs"
				" (NotCod,PublisherCod,PubType,TimePublish)"
				" VALUES"
				" (%ld,%ld,%u,NOW())",
				SocPub->NotCod,
				SocPub->PublisherCod,
				(unsigned) SocPub->PubType);

   /***** Increment number of publications in user's figures *****/
   Prf_IncrementNumSocPubUsr (SocPub->PublisherCod);
  }

/*****************************************************************************/
/********************** Form to write a new publication **********************/
/*****************************************************************************/

static void TL_PutFormToWriteNewPost (void)
  {
   extern const char *Txt_New_TIMELINE_post;
   bool ShowPhoto;
   char PhotoURL[PATH_MAX + 1];

   /***** Start list *****/
   fprintf (Gbl.F.Out,"<ul class=\"TL_LIST\">"
                      "<li class=\"TL_WIDTH\">");

   /***** Left: write author's photo (my photo) *****/
   fprintf (Gbl.F.Out,"<div class=\"TL_LEFT_PHOTO\">");
   ShowPhoto = Pho_ShowingUsrPhotoIsAllowed (&Gbl.Usrs.Me.UsrDat,PhotoURL);
   Pho_ShowUsrPhoto (&Gbl.Usrs.Me.UsrDat,ShowPhoto ? PhotoURL :
						     NULL,
		     "PHOTO42x56",Pho_ZOOM,false);
   fprintf (Gbl.F.Out,"</div>");

   /***** Right: author's name, time, summary and buttons *****/
   fprintf (Gbl.F.Out,"<div class=\"TL_RIGHT_CONTAINER TL_RIGHT_WIDTH\">");

   /* Write author's full name and nickname */
   fprintf (Gbl.F.Out,"<div class=\"TL_RIGHT_AUTHOR TL_RIGHT_AUTHOR_WIDTH\">"
		      "<span class=\"DAT_N_BOLD\">%s</span>"
		      "<span class=\"DAT_LIGHT\"> @%s</span>"
		      "</div>",
	    Gbl.Usrs.Me.UsrDat.FullName,Gbl.Usrs.Me.UsrDat.Nickname);

   /***** Form to write the post *****/
   /* Start container */
   fprintf (Gbl.F.Out,"<div class=\"TL_FORM_NEW_POST TL_RIGHT_WIDTH\">");

   /* Start form to write the post */
   TL_FormStart (ActRcvSocPstGbl,ActRcvSocPstUsr);

   /* Textarea and button */
   TL_PutTextarea (Txt_New_TIMELINE_post,
                    "TL_POST_TEXTAREA TL_RIGHT_WIDTH",
		    "TL_POST_IMG_TIT_URL TL_POST_IMG_WIDTH");

   /* End form */
   Frm_EndForm ();

   /* End container */
   fprintf (Gbl.F.Out,"</div>");

   /***** End list *****/
   fprintf (Gbl.F.Out,"</li>"
	              "</ul>");
  }

/*****************************************************************************/
/*** Put textarea and button inside a form to submit a new post or comment ***/
/*****************************************************************************/

static void TL_PutTextarea (const char *Placeholder,
                            const char *ClassTextArea,const char *ClassImgTit)
  {
   extern const char *Txt_Post;
   char IdDivImgButton[Frm_MAX_BYTES_ID + 1];

   /***** Set unique id for the hidden div *****/
   Frm_SetUniqueId (IdDivImgButton);

   /***** Textarea to write the content *****/
   fprintf (Gbl.F.Out,"<textarea name=\"Content\" rows=\"1\" maxlength=\"%u\""
                      " placeholder=\"%s&hellip;\""
	              " class=\"%s\""
	              " onfocus=\"expandTextarea(this,'%s','5');\">"
		      "</textarea>",
            TL_MAX_CHARS_IN_POST,
            Placeholder,ClassTextArea,
            IdDivImgButton);

   /***** Start concealable div *****/
   fprintf (Gbl.F.Out,"<div id=\"%s\" style=\"display:none;\">",
            IdDivImgButton);

   /***** Help on editor *****/
   Lay_HelpPlainEditor ();

   /***** Attached image (optional) *****/
   Med_PutMediaUploader (-1,ClassImgTit);

   /***** Submit button *****/
   fprintf (Gbl.F.Out,"<button type=\"submit\""
	              " class=\"BT_SUBMIT_INLINE BT_CREATE\">"
		      "%s"
		      "</button>",
	    Txt_Post);

   /***** End hidden div *****/
   fprintf (Gbl.F.Out,"</div>");
  }

/*****************************************************************************/
/******************* Receive and store a new public post *********************/
/*****************************************************************************/

void TL_ReceivePostGbl (void)
  {
   long NotCod;

   /***** Receive and store post *****/
   NotCod = TL_ReceivePost ();

   /***** Write updated timeline after publication (global) *****/
   TL_ShowTimelineGblHighlightingNot (NotCod);
  }

void TL_ReceivePostUsr (void)
  {
   long NotCod;

   /***** Get user whom profile is displayed *****/
   Usr_GetParamOtherUsrCodEncryptedAndGetUsrData ();

   /***** Show user's profile *****/
   Prf_ShowUserProfile (&Gbl.Usrs.Other.UsrDat);

   /***** Start section *****/
   Lay_StartSection (TL_TIMELINE_SECTION_ID);

   /***** Receive and store post *****/
   NotCod = TL_ReceivePost ();

   /***** Write updated timeline after publication (user) *****/
   TL_ShowTimelineUsrHighlightingNot (NotCod);

   /***** End section *****/
   Lay_EndSection ();
  }

// Returns the code of the note just created
static long TL_ReceivePost (void)
  {
   char Content[Cns_MAX_BYTES_LONG_TEXT + 1];
   struct Media Media;
   long PstCod;
   struct TL_Publication SocPub;

   /***** Get the content of the new post *****/
   Par_GetParAndChangeFormat ("Content",Content,Cns_MAX_BYTES_LONG_TEXT,
                              Str_TO_RIGOROUS_HTML,true);

   /***** Initialize image *****/
   Med_MediaConstructor (&Media);

   /***** Get attached image (action, file and title) *****/
   Media.Width   = TL_IMAGE_SAVED_MAX_WIDTH;
   Media.Height  = TL_IMAGE_SAVED_MAX_HEIGHT;
   Media.Quality = TL_IMAGE_SAVED_QUALITY;
   Med_GetMediaFromForm (-1,&Media,NULL);

   if (Content[0] ||			// Text not empty
       Media.Status == Med_PROCESSED)	// A media is attached
     {
      /***** Check if image is received and processed *****/
      if (Media.Action == Med_ACTION_NEW_MEDIA &&	// Upload new image
	  Media.Status == Med_PROCESSED)		// The new image received has been processed
	 /* Move processed image to definitive directory */
	 Med_MoveMediaToDefinitiveDir (&Media);

      /***** Publish *****/
      /* Insert post content in the database */
      PstCod =
      DB_QueryINSERTandReturnCode ("can not create post",
				   "INSERT INTO social_posts"
				   " (Content,"
				   "MediaName,MediaType,MediaTitle,MediaURL)"
				   " VALUES"
				   " ('%s',"
				   "'%s','%s','%s','%s')",
				   Content,
				   Media.Name,
				   Med_GetStringTypeForDB (Media.Type),
				   Media.Title ? Media.Title : "",
				   Media.URL   ? Media.URL   : "");

      /* Insert post in notes */
      TL_StoreAndPublishNote (TL_NOTE_POST,PstCod,&SocPub);

      /***** Analyze content and store notifications about mentions *****/
      Str_AnalyzeTxtAndStoreNotifyEventToMentionedUsrs (SocPub.PubCod,Content);
     }
   else	// Text and image are empty
      SocPub.NotCod = -1L;

   /***** Free image *****/
   Med_MediaDestructor (&Media);

   return SocPub.NotCod;
  }

/*****************************************************************************/
/********* Put an icon to toggle on/off the form to comment a note ***********/
/*****************************************************************************/

static void TL_PutIconToToggleCommentNote (const char UniqueId[Frm_MAX_BYTES_ID + 1])
  {
   extern const char *Txt_Comment;

   /***** Link to toggle on/off the form to comment a note *****/
   fprintf (Gbl.F.Out,"<div class=\"TL_ICO_COMMENT ICO_HIGHLIGHT\">"
                      "<a href=\"\""
                      " onclick=\"toggleDisplay('%s');return false;\" />"
                      "<img src=\"%s/edit.svg\""
                      " alt=\"%s\" title=\"%s\""
                      " class=\"CONTEXT_ICO_x16\" />"
                      "</a>"
                      "</div>",
            UniqueId,
            Gbl.Prefs.URLIcons,
            Txt_Comment,Txt_Comment);
  }

/*****************************************************************************/
/********** Put an icon to toggle on/off the form to comment a note **********/
/*****************************************************************************/

static void TL_PutIconCommentDisabled (void)
  {
   extern const char *Txt_Comment;

   /***** Disabled icon to comment a note *****/
   fprintf (Gbl.F.Out,"<div class=\"TL_ICO_COMMENT_DISABLED\">"
 		      "<img src=\"%s/edit.svg\""
		      " alt=\"%s\" title=\"%s\""
		      " class=\"ICO16x16\" />"
		      "</div>",
	    Gbl.Prefs.URLIcons,
	    Txt_Comment,Txt_Comment);
  }

/*****************************************************************************/
/********************** Form to comment a publication ************************/
/*****************************************************************************/
// All forms in this function and nested functions must have unique identifiers

static void TL_PutHiddenFormToWriteNewCommentToNote (long NotCod,
                                                     const char IdNewComment[Frm_MAX_BYTES_ID + 1])
  {
   extern const char *Txt_New_TIMELINE_comment;
   bool ShowPhoto = false;
   char PhotoURL[PATH_MAX + 1];

   /***** Start container *****/
   fprintf (Gbl.F.Out,"<div id=\"%s\""
		      " class=\"TL_FORM_NEW_COMMENT TL_RIGHT_WIDTH\""
		      " style=\"display:none;\">",
	    IdNewComment);

   /***** Left: write author's photo (my photo) *****/
   fprintf (Gbl.F.Out,"<div class=\"TL_COMMENT_PHOTO\">");
   ShowPhoto = Pho_ShowingUsrPhotoIsAllowed (&Gbl.Usrs.Me.UsrDat,PhotoURL);
   Pho_ShowUsrPhoto (&Gbl.Usrs.Me.UsrDat,ShowPhoto ? PhotoURL :
					             NULL,
		     "PHOTO30x40",Pho_ZOOM,true);	// Use unique id
   fprintf (Gbl.F.Out,"</div>");

   /***** Right: form to write the comment *****/
   /* Start right container */
   fprintf (Gbl.F.Out,"<div class=\"TL_COMMENT_CONTAINER TL_COMMENT_WIDTH\">");

   /* Start form to write the post */
   TL_FormStart (ActRcvSocComGbl,ActRcvSocComUsr);
   TL_PutHiddenParamNotCod (NotCod);

   /* Textarea and button */
   TL_PutTextarea (Txt_New_TIMELINE_comment,
                    "TL_COMMENT_TEXTAREA TL_COMMENT_WIDTH",
		    "TL_COMMENT_IMG_TIT_URL TL_COMMENT_WIDTH");

   /* End form */
   Frm_EndForm ();

   /* End right container */
   fprintf (Gbl.F.Out,"</div>");

   /***** End container *****/
   fprintf (Gbl.F.Out,"</div>");
  }

/*****************************************************************************/
/********************* Get number of comments in a note **********************/
/*****************************************************************************/

static unsigned long TL_GetNumCommentsInNote (long NotCod)
  {
   return DB_QueryCOUNT ("can not get number of comments in a note",
			 "SELECT COUNT(*) FROM social_pubs"
			 " WHERE NotCod=%ld AND PubType=%u",
			 NotCod,(unsigned) TL_PUB_COMMENT_TO_NOTE);
  }

/*****************************************************************************/
/*********************** Write comments in a note ****************************/
/*****************************************************************************/
// All forms in this function and nested functions must have unique identifiers

static void TL_WriteCommentsInNote (const struct TL_Note *SocNot)
  {
   extern const char *Txt_See_the_previous_X_COMMENTS;
   extern const char *Txt_See_only_the_latest_COMMENTS;
   MYSQL_RES *mysql_res;
   unsigned long NumComments;
   unsigned long NumCommentsInitiallyHidden;
   unsigned long NumCom;
   char IdComments[Frm_MAX_BYTES_ID + 1];

   /***** Get comments of this note from database *****/
   NumComments = DB_QuerySELECT (&mysql_res,"can not get comments",
				 "SELECT social_pubs.PubCod,"		// row[0]
				        "social_pubs.PublisherCod,"	// row[1]
				        "social_pubs.NotCod,"		// row[2]
				        "UNIX_TIMESTAMP("
				        "social_pubs.TimePublish),"	// row[3]
					"social_comments.Content,"	// row[4]
				        "social_comments.MediaName,"	// row[5]
				        "social_comments.MediaType,"	// row[6]
					"social_comments.MediaTitle,"	// row[7]
				        "social_comments.MediaURL"	// row[8]
				 " FROM social_pubs,social_comments"
				 " WHERE social_pubs.NotCod=%ld"
				 " AND social_pubs.PubType=%u"
				 " AND social_pubs.PubCod=social_comments.PubCod"
				 " ORDER BY social_pubs.PubCod",
				 SocNot->NotCod,(unsigned) TL_PUB_COMMENT_TO_NOTE);

   /***** List comments *****/
   if (NumComments)	// Comments to this note found
     {
      // Never hide only one comment
      // So, the number of comments initially hidden must be 0 or >= 2
      NumCommentsInitiallyHidden = (NumComments <= TL_NUM_VISIBLE_COMMENTS + 1) ? 0 :	// Show all
	                        					          NumComments - TL_NUM_VISIBLE_COMMENTS;
      if (NumCommentsInitiallyHidden)
        {
	 /***** Create unique id for list of hidden comments *****/
	 Frm_SetUniqueId (IdComments);

	 /***** Link to toggle on/off comments *****/
	 fprintf (Gbl.F.Out,"<div id=\"con_%s\""
			    " class=\"TL_EXPAND_COMMENTS TL_RIGHT_WIDTH\""
			    " style=\"display:none;\">",	// Initially hidden
		  IdComments);
	 TL_PutIconToToggleComments (IdComments,"angle-down.svg",
	                              Txt_See_only_the_latest_COMMENTS);
	 fprintf (Gbl.F.Out,"</div>");

	 /***** First list with comments initially hidden *****/
	 fprintf (Gbl.F.Out,"<ul id=\"com_%s\" class=\"LIST_LEFT\""
			    " style=\"display:none;\">",	// Initially hidden
		  IdComments);
	 for (NumCom = 0;
	      NumCom < NumCommentsInitiallyHidden;
	      NumCom++)
	    TL_WriteOneCommentInList (mysql_res);
	 fprintf (Gbl.F.Out,"</ul>");

	 /***** Link to toggle on/off comments *****/
	 fprintf (Gbl.F.Out,"<div id=\"exp_%s\""
			    " class=\"TL_EXPAND_COMMENTS TL_RIGHT_WIDTH\">",
		  IdComments);
	 snprintf (Gbl.Title,sizeof (Gbl.Title),
		   Txt_See_the_previous_X_COMMENTS,
		   NumCommentsInitiallyHidden);
	 TL_PutIconToToggleComments (IdComments,"angle-up.svg",Gbl.Title);
	 fprintf (Gbl.F.Out,"</div>");
        }

      /***** Second list with comments initially visible *****/
      fprintf (Gbl.F.Out,"<ul class=\"LIST_LEFT\">");
      for (NumCom = NumCommentsInitiallyHidden;
	   NumCom < NumComments;
	   NumCom++)
	 TL_WriteOneCommentInList (mysql_res);
      fprintf (Gbl.F.Out,"</ul>");
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

static void TL_WriteOneCommentInList (MYSQL_RES *mysql_res)
  {
   MYSQL_ROW row;
   struct TL_Comment SocCom;

   /***** Initialize image *****/
   Med_MediaConstructor (&SocCom.Media);

   /***** Get data of comment *****/
   row = mysql_fetch_row (mysql_res);
   TL_GetDataOfCommentFromRow (row,&SocCom);

   /***** Write comment *****/
   TL_WriteComment (&SocCom,
			   TL_TOP_MESSAGE_NONE,-1L,
			   false);

   /***** Free image *****/
   Med_MediaDestructor (&SocCom.Media);
  }

/*****************************************************************************/
/********** Put an icon to toggle on/off comments in a publication ***********/
/*****************************************************************************/

static void TL_PutIconToToggleComments (const char *UniqueId,
                                        const char *Icon,const char *Text)
  {
   extern const char *The_ClassFormInBox[The_NUM_THEMES];

   /***** Link to toggle on/off some divs *****/
   fprintf (Gbl.F.Out,"<a href=\"\" title=\"%s\" class=\"%s\""
                      " onclick=\"toggleComments('%s');"
                                 "return false;\" />",
            Text,The_ClassFormInBox[Gbl.Prefs.Theme],
            UniqueId);
   Ico_PutIconTextLink (Icon,Text);
   fprintf (Gbl.F.Out,"</a>");
  }

/*****************************************************************************/
/******************************** Write comment ******************************/
/*****************************************************************************/
// All forms in this function and nested functions must have unique identifiers

static void TL_WriteComment (struct TL_Comment *SocCom,
                             TL_TopMessage_t TopMessage,long UsrCod,
                             bool ShowCommentAlone)	// Comment is shown alone, not in a list
  {
   extern const char *Txt_Forum;
   extern const char *Txt_Course;
   extern const char *Txt_Degree;
   extern const char *Txt_Centre;
   extern const char *Txt_Institution;
   struct UsrData UsrDat;
   bool ItsMe;
   bool IAmTheAuthor;
   bool IAmAFaverOfThisSocCom = false;
   bool ShowPhoto = false;
   char PhotoURL[PATH_MAX + 1];
   static unsigned NumDiv = 0;	// Used to create unique div id for fav

   NumDiv++;

   if (ShowCommentAlone)
     {
      Box_StartBox (NULL,NULL,NULL,
                    NULL,Box_NOT_CLOSABLE);

      /***** Write sharer/commenter if distinct to author *****/
      TL_WriteTopMessage (TopMessage,UsrCod);

      fprintf (Gbl.F.Out,"<div class=\"TL_LEFT_PHOTO\">"
                         "</div>"
                         "<div class=\"TL_RIGHT_CONTAINER TL_RIGHT_WIDTH\">"
                         "<ul class=\"LIST_LEFT\">");
     }

   /***** Start list item *****/
   fprintf (Gbl.F.Out,"<li");
   if (!ShowCommentAlone)
      fprintf (Gbl.F.Out," class=\"TL_COMMENT\"");
   fprintf (Gbl.F.Out,">");

   if (SocCom->PubCod <= 0 ||
       SocCom->NotCod <= 0 ||
       SocCom->UsrCod <= 0)
      Ale_ShowAlert (Ale_ERROR,"Error in comment.");
   else
     {
      /***** Get author's data *****/
      Usr_UsrDataConstructor (&UsrDat);
      UsrDat.UsrCod = SocCom->UsrCod;
      Usr_ChkUsrCodAndGetAllUsrDataFromUsrCod (&UsrDat);
      ItsMe = Usr_ItsMe (UsrDat.UsrCod);
      IAmTheAuthor = ItsMe;
      if (!IAmTheAuthor)
	 IAmAFaverOfThisSocCom = TL_CheckIfCommIsFavedByUsr (SocCom->PubCod,
							      Gbl.Usrs.Me.UsrDat.UsrCod);

      /***** Left: write author's photo *****/
      fprintf (Gbl.F.Out,"<div class=\"TL_COMMENT_PHOTO\">");
      ShowPhoto = Pho_ShowingUsrPhotoIsAllowed (&UsrDat,PhotoURL);
      Pho_ShowUsrPhoto (&UsrDat,ShowPhoto ? PhotoURL :
					    NULL,
			"PHOTO30x40",Pho_ZOOM,true);	// Use unique id
      fprintf (Gbl.F.Out,"</div>");

      /***** Right: author's name, time, content, image and buttons *****/
      fprintf (Gbl.F.Out,"<div class=\"TL_COMMENT_CONTAINER TL_COMMENT_WIDTH\">");

      /* Write author's full name and nickname */
      TL_WriteAuthorComment (&UsrDat);

      /* Write date and time */
      TL_WriteDateTime (SocCom->DateTimeUTC);

      /* Write content of the comment */
      fprintf (Gbl.F.Out,"<div class=\"TL_TXT\">");
      Msg_WriteMsgContent (SocCom->Content,Cns_MAX_BYTES_LONG_TEXT,true,false);
      fprintf (Gbl.F.Out,"</div>");

      /* Show image */
      Med_ShowMedia (&SocCom->Media,"TL_COMMENT_IMG_CONTAINER TL_COMMENT_WIDTH",
	                            "TL_COMMENT_IMG TL_COMMENT_WIDTH");

      /* Put icon to mark this comment as favourite */
      if (IAmTheAuthor)			// I am the author
        {
	 /* Put disabled icon and list of users
	    who have marked this comment as favourite */
	 TL_PutDisabledIconFav (SocCom->NumFavs);
         TL_ShowUsrsWhoHaveMarkedCommAsFav (SocCom);
        }
      else				// I am not the author
	{
	 fprintf (Gbl.F.Out,"<div id=\"fav_com_%s_%u\""
	                    " class=\"TL_ICO_FAV\">",
	          Gbl.UniqueNameEncrypted,NumDiv);
	 if (IAmAFaverOfThisSocCom)	// I have favourited this comment
	    /* Put icon to unfav this publication and list of users */
	    TL_PutFormToUnfavComment (SocCom);
	 else				// I am not a favouriter
	    /* Put icon to fav this publication and list of users */
	    TL_PutFormToFavComment (SocCom);
	 fprintf (Gbl.F.Out,"</div>");
	}

      /* Put icon to remove this comment */
      if (IAmTheAuthor && !ShowCommentAlone)
	 TL_PutFormToRemoveComment (SocCom->PubCod);

      /***** Free memory used for user's data *****/
      Usr_UsrDataDestructor (&UsrDat);
     }

   /***** End list item *****/
   fprintf (Gbl.F.Out,"</li>");

   if (ShowCommentAlone)
     {
      fprintf (Gbl.F.Out,"</ul>"
                         "</div>");
      Box_EndBox ();
     }
  }

/*****************************************************************************/
/********* Write name and nickname of author of a comment to a note **********/
/*****************************************************************************/
// All forms in this function and nested functions must have unique identifiers

static void TL_WriteAuthorComment (struct UsrData *UsrDat)
  {
   extern const char *Txt_My_public_profile;
   extern const char *Txt_Another_user_s_profile;
   bool ItsMe = Usr_ItsMe (UsrDat->UsrCod);

   fprintf (Gbl.F.Out,"<div class=\"TL_COMMENT_AUTHOR TL_COMMENT_AUTHOR_WIDTH\">");

   /***** Show user's name inside form to go to user's public profile *****/
   Frm_StartFormUnique (ActSeeOthPubPrf);
   Usr_PutParamUsrCodEncrypted (UsrDat->EncryptedUsrCod);
   Frm_LinkFormSubmitUnique (ItsMe ? Txt_My_public_profile :
				     Txt_Another_user_s_profile,
			     "DAT_BOLD");
   fprintf (Gbl.F.Out,"%s</a>",UsrDat->FullName);
   Frm_EndForm ();

   /***** Show user's nickname inside form to go to user's public profile *****/
   Frm_StartFormUnique (ActSeeOthPubPrf);
   Usr_PutParamUsrCodEncrypted (UsrDat->EncryptedUsrCod);
   Frm_LinkFormSubmitUnique (ItsMe ? Txt_My_public_profile :
				     Txt_Another_user_s_profile,
			     "DAT_LIGHT");
   fprintf (Gbl.F.Out," @%s</a>",UsrDat->Nickname);
   Frm_EndForm ();

   fprintf (Gbl.F.Out,"</div>");
  }

/*****************************************************************************/
/************************* Form to remove comment ****************************/
/*****************************************************************************/
// All forms in this function and nested functions must have unique identifiers

static void TL_PutFormToRemoveComment (long PubCod)
  {
   extern const char *Txt_Remove;

   /***** Form to remove publication *****/
   TL_FormStart (ActReqRemSocComGbl,ActReqRemSocComUsr);
   TL_PutHiddenParamPubCod (PubCod);
   Ico_PutDivIconLink ("TL_ICO_REM",
		       "trash.svg",Txt_Remove);
   Frm_EndForm ();
  }

/*****************************************************************************/
/*********************** Put disabled icon to share **************************/
/*****************************************************************************/

static void TL_PutDisabledIconShare (unsigned NumShared)
  {
   extern const char *Txt_TIMELINE_NOTE_Shared_by_X_USERS;
   extern const char *Txt_TIMELINE_NOTE_Not_shared_by_anyone;

   if (NumShared)
      snprintf (Gbl.Title,sizeof (Gbl.Title),
	        Txt_TIMELINE_NOTE_Shared_by_X_USERS,
		NumShared);
   else
      Str_Copy (Gbl.Title,Txt_TIMELINE_NOTE_Not_shared_by_anyone,
                Lay_MAX_BYTES_TITLE);

   /***** Disabled icon to share *****/
   Ico_PutDivIcon ("TL_ICO_SHA_DISABLED",
		   "share-alt.svg",Gbl.Title);
  }

/*****************************************************************************/
/****************** Put disabled icon to mark as favourite *******************/
/*****************************************************************************/

static void TL_PutDisabledIconFav (unsigned NumFavs)
  {
   extern const char *Txt_TIMELINE_NOTE_Favourited_by_X_USERS;
   extern const char *Txt_TIMELINE_NOTE_Not_favourited_by_anyone;

   if (NumFavs)
      snprintf (Gbl.Title,sizeof (Gbl.Title),
	        Txt_TIMELINE_NOTE_Favourited_by_X_USERS,
		NumFavs);
   else
      Str_Copy (Gbl.Title,Txt_TIMELINE_NOTE_Not_favourited_by_anyone,
                Lay_MAX_BYTES_TITLE);

   /***** Disabled icon to mark as favourite *****/
   Ico_PutDivIcon ("TL_ICO_FAV_DISABLED",
		   "heart.svg",Gbl.Title);
  }

/*****************************************************************************/
/***************************** Form to share note ****************************/
/*****************************************************************************/
// All forms in this function and nested functions must have unique identifiers

static void TL_PutFormToShareNote (const struct TL_Note *SocNot)
  {
   extern const char *Txt_Share;
   char ParamCod[6 + 1 + 10 + 1];

   /***** Form and icon to mark note as favourite *****/
   sprintf (ParamCod,"NotCod=%ld",SocNot->NotCod);
   TL_FormFavSha (ActShaSocNotGbl,ActShaSocNotUsr,ParamCod,
	           "share-alt.svg",Txt_Share);

   /***** Who have shared this note *****/
   TL_ShowUsrsWhoHaveSharedNote (SocNot);
  }

/*****************************************************************************/
/****************** Form to unshare (stop sharing) note **********************/
/*****************************************************************************/
// All forms in this function and nested functions must have unique identifiers

static void TL_PutFormToUnshareNote (const struct TL_Note *SocNot)
  {
   extern const char *Txt_TIMELINE_NOTE_Shared;
   char ParamCod[6 + 1 + 10 + 1];

   /***** Form and icon to mark note as favourite *****/
   sprintf (ParamCod,"NotCod=%ld",SocNot->NotCod);
   TL_FormFavSha (ActUnsSocNotGbl,ActUnsSocNotUsr,ParamCod,
	           "share-alt-green.svg",Txt_TIMELINE_NOTE_Shared);

   /***** Who have shared this note *****/
   TL_ShowUsrsWhoHaveSharedNote (SocNot);
  }

/*****************************************************************************/
/******************* Form to fav (mark as favourite) note ********************/
/*****************************************************************************/
// All forms in this function and nested functions must have unique identifiers

static void TL_PutFormToFavNote (const struct TL_Note *SocNot)
  {
   extern const char *Txt_Mark_as_favourite;
   char ParamCod[6 + 1 + 10 + 1];

   /***** Form and icon to mark note as favourite *****/
   sprintf (ParamCod,"NotCod=%ld",SocNot->NotCod);
   TL_FormFavSha (ActFavSocNotGbl,ActFavSocNotUsr,ParamCod,
	           "heart.svg",Txt_Mark_as_favourite);

   /***** Who have marked this note as favourite *****/
   TL_ShowUsrsWhoHaveMarkedNoteAsFav (SocNot);
  }

/*****************************************************************************/
/*************** Form to unfav (remove mark as favourite) note ***************/
/*****************************************************************************/
// All forms in this function and nested functions must have unique identifiers

static void TL_PutFormToUnfavNote (const struct TL_Note *SocNot)
  {
   extern const char *Txt_TIMELINE_NOTE_Favourite;
   char ParamCod[6 + 1 + 10 + 1];

   /***** Form and icon to unfav (remove mark as favourite) note *****/
   sprintf (ParamCod,"NotCod=%ld",SocNot->NotCod);
   TL_FormFavSha (ActUnfSocNotGbl,ActUnfSocNotUsr,ParamCod,
	           "heart-red.svg",Txt_TIMELINE_NOTE_Favourite);

   /***** Who have marked this note as favourite *****/
   TL_ShowUsrsWhoHaveMarkedNoteAsFav (SocNot);
  }

/*****************************************************************************/
/********************* Form to mark a comment as favourite *******************/
/*****************************************************************************/
// All forms in this function and nested functions must have unique identifiers

static void TL_PutFormToFavComment (struct TL_Comment *SocCom)
  {
   extern const char *Txt_Mark_as_favourite;
   char ParamCod[6 + 1 + 10 + 1];

   /***** Form and icon to mark comment as favourite *****/
   sprintf (ParamCod,"PubCod=%ld",SocCom->PubCod);
   TL_FormFavSha (ActFavSocComGbl,ActFavSocComUsr,ParamCod,
	           "heart.svg",Txt_Mark_as_favourite);

   /***** Show who have marked this comment as favourite *****/
   TL_ShowUsrsWhoHaveMarkedCommAsFav (SocCom);
  }

/*****************************************************************************/
/************* Form to unfav (remove mark as favourite) comment **************/
/*****************************************************************************/
// All forms in this function and nested functions must have unique identifiers

static void TL_PutFormToUnfavComment (struct TL_Comment *SocCom)
  {
   extern const char *Txt_TIMELINE_NOTE_Favourite;
   char ParamCod[6 + 1 + 10 + 1];

   /***** Form and icon to unfav (remove mark as favourite) comment *****/
   sprintf (ParamCod,"PubCod=%ld",SocCom->PubCod);
   TL_FormFavSha (ActUnfSocComGbl,ActUnfSocComUsr,ParamCod,
	           "heart-red.svg",Txt_TIMELINE_NOTE_Favourite);

   /***** Show who have marked this comment as favourite *****/
   TL_ShowUsrsWhoHaveMarkedCommAsFav (SocCom);
  }

/*****************************************************************************/
/************************ Form to remove publication *************************/
/*****************************************************************************/
// All forms in this function and nested functions must have unique identifiers

static void TL_PutFormToRemovePublication (long NotCod)
  {
   extern const char *Txt_Remove;

   /***** Form to remove publication *****/
   TL_FormStart (ActReqRemSocPubGbl,ActReqRemSocPubUsr);
   TL_PutHiddenParamNotCod (NotCod);
   Ico_PutDivIconLink ("TL_ICO_REM",
		       "trash.svg",Txt_Remove);
   Frm_EndForm ();
  }

/*****************************************************************************/
/****************** Put parameter with the code of a note ********************/
/*****************************************************************************/

static void TL_PutHiddenParamNotCod (long NotCod)
  {
   Par_PutHiddenParamLong ("NotCod",NotCod);
  }

/*****************************************************************************/
/*************** Put parameter with the code of a publication ****************/
/*****************************************************************************/

void TL_PutHiddenParamPubCod (long PubCod)
  {
   Par_PutHiddenParamLong ("PubCod",PubCod);
  }

/*****************************************************************************/
/****************** Get parameter with the code of a note ********************/
/*****************************************************************************/

static long TL_GetParamNotCod (void)
  {
   /***** Get note code *****/
   return Par_GetParToLong ("NotCod");
  }

/*****************************************************************************/
/**************** Get parameter with the code of a publication ***************/
/*****************************************************************************/

static long TL_GetParamPubCod (void)
  {
   /***** Get comment code *****/
   return Par_GetParToLong ("PubCod");
  }

/*****************************************************************************/
/******************************* Comment a note ******************************/
/*****************************************************************************/

void TL_ReceiveCommentGbl (void)
  {
   long NotCod;

   /***** Receive comment in a note *****/
   NotCod = TL_ReceiveComment ();

   /***** Write updated timeline after commenting (global) *****/
   TL_ShowTimelineGblHighlightingNot (NotCod);
  }

void TL_ReceiveCommentUsr (void)
  {
   long NotCod;

   /***** Get user whom profile is displayed *****/
   Usr_GetParamOtherUsrCodEncryptedAndGetUsrData ();

   /***** Show user's profile *****/
   Prf_ShowUserProfile (&Gbl.Usrs.Other.UsrDat);

   /***** Start section *****/
   Lay_StartSection (TL_TIMELINE_SECTION_ID);

   /***** Receive comment in a note *****/
   NotCod = TL_ReceiveComment ();

   /***** Write updated timeline after commenting (user) *****/
   TL_ShowTimelineUsrHighlightingNot (NotCod);

   /***** End section *****/
   Lay_EndSection ();
  }

static long TL_ReceiveComment (void)
  {
   extern const char *Txt_The_original_post_no_longer_exists;
   char Content[Cns_MAX_BYTES_LONG_TEXT + 1];
   struct Media Media;
   struct TL_Note SocNot;
   struct TL_Publication SocPub;

   /***** Get data of note *****/
   SocNot.NotCod = TL_GetParamNotCod ();
   TL_GetDataOfNoteByCod (&SocNot);

   if (SocNot.NotCod > 0)
     {
      /***** Get the content of the comment *****/
      Par_GetParAndChangeFormat ("Content",Content,Cns_MAX_BYTES_LONG_TEXT,
				 Str_TO_RIGOROUS_HTML,true);

      /***** Initialize image *****/
      Med_MediaConstructor (&Media);

      /***** Get attached image (action, file and title) *****/
      Media.Width   = TL_IMAGE_SAVED_MAX_WIDTH;
      Media.Height  = TL_IMAGE_SAVED_MAX_HEIGHT;
      Media.Quality = TL_IMAGE_SAVED_QUALITY;
      Med_GetMediaFromForm (-1,&Media,NULL);

      if (Content[0] ||				// Text not empty
	  Media.Status == Med_PROCESSED)	// A media is attached
	{
	 /***** Check if image is received and processed *****/
	 if (Media.Action == Med_ACTION_NEW_MEDIA &&	// Upload new image
	     Media.Status == Med_PROCESSED)	// The new image received has been processed
	    /* Move processed image to definitive directory */
	    Med_MoveMediaToDefinitiveDir (&Media);

	 /***** Publish *****/
	 /* Insert into publications */
	 SocPub.NotCod       = SocNot.NotCod;
	 SocPub.PublisherCod = Gbl.Usrs.Me.UsrDat.UsrCod;
	 SocPub.PubType      = TL_PUB_COMMENT_TO_NOTE;
	 TL_PublishNoteInTimeline (&SocPub);	// Set SocPub.PubCod

	 /* Insert comment content in the database */
	 DB_QueryINSERT ("can not store comment content",
			 "INSERT INTO social_comments"
	                 " (PubCod,Content,"
	                 "MediaName,MediaType,MediaTitle,MediaURL)"
			 " VALUES"
			 " (%ld,'%s',"
			 "'%s','%s','%s','%s')",
			 SocPub.PubCod,
			 Content,
			 Media.Name,
			 Med_GetStringTypeForDB (Media.Type),
			 Media.Title ? Media.Title : "",
			 Media.URL   ? Media.URL   : "");

	 /***** Store notifications about the new comment *****/
	 Ntf_StoreNotifyEventsToAllUsrs (Ntf_EVENT_TIMELINE_COMMENT,SocPub.PubCod);

	 /***** Analyze content and store notifications about mentions *****/
	 Str_AnalyzeTxtAndStoreNotifyEventToMentionedUsrs (SocPub.PubCod,Content);
	}

      /***** Free image *****/
      Med_MediaDestructor (&Media);
     }
   else
      Ale_ShowAlert (Ale_WARNING,Txt_The_original_post_no_longer_exists);

   return SocNot.NotCod;
  }

/*****************************************************************************/
/******************************** Share a note *******************************/
/*****************************************************************************/

void TL_ShareNoteGbl (void)
  {
   struct TL_Note SocNot;

   /***** Share note *****/
   TL_ShareNote (&SocNot);

   /***** Write HTML inside DIV with form to unshare *****/
   TL_PutFormToUnshareNote (&SocNot);

   /***** All the output is made, so don't write anymore *****/
   Gbl.Layout.DivsEndWritten = Gbl.Layout.HTMLEndWritten = true;
  }

void TL_ShareNoteUsr (void)
  {
   struct TL_Note SocNot;

   /***** Get user whom profile is displayed *****/
   Usr_GetParamOtherUsrCodEncryptedAndGetUsrData ();

   /***** Share note *****/
   TL_ShareNote (&SocNot);

   /***** Write HTML inside DIV with form to unshare *****/
   TL_PutFormToUnshareNote (&SocNot);

   /***** All the output is made, so don't write anymore *****/
   Gbl.Layout.DivsEndWritten = Gbl.Layout.HTMLEndWritten = true;
  }

static void TL_ShareNote (struct TL_Note *SocNot)
  {
   extern const char *Txt_The_original_post_no_longer_exists;
   struct TL_Publication SocPub;
   bool ItsMe;
   long OriginalPubCod;

   /***** Get data of note *****/
   SocNot->NotCod = TL_GetParamNotCod ();
   TL_GetDataOfNoteByCod (SocNot);

   if (SocNot->NotCod > 0)
     {
      ItsMe = Usr_ItsMe (SocNot->UsrCod);
      if (Gbl.Usrs.Me.Logged && !ItsMe)	// I am not the author
         if (!TL_CheckIfNoteIsSharedByUsr (SocNot->NotCod,
					    Gbl.Usrs.Me.UsrDat.UsrCod))	// Not yet shared by me
	   {
	    /***** Share (publish note in timeline) *****/
	    SocPub.NotCod       = SocNot->NotCod;
	    SocPub.PublisherCod = Gbl.Usrs.Me.UsrDat.UsrCod;
	    SocPub.PubType      = TL_PUB_SHARED_NOTE;
	    TL_PublishNoteInTimeline (&SocPub);	// Set SocPub.PubCod

	    /* Update number of times this note is shared */
	    TL_UpdateNumTimesANoteHasBeenShared (SocNot);

	    /**** Create notification about shared post
		  for the author of the post ***/
	    OriginalPubCod = TL_GetPubCodOfOriginalNote (SocNot->NotCod);
	    if (OriginalPubCod > 0)
	       TL_CreateNotifToAuthor (SocNot->UsrCod,OriginalPubCod,Ntf_EVENT_TIMELINE_SHARE);
	   }
     }
  }

/*****************************************************************************/
/************************** Mark a note as favourite *************************/
/*****************************************************************************/

void TL_FavNoteGbl (void)
  {
   struct TL_Note SocNot;

   /***** Mark note as favourite *****/
   TL_FavNote (&SocNot);

   /***** Write HTML inside DIV with form to unfav *****/
   TL_PutFormToUnfavNote (&SocNot);

   /***** All the output is made, so don't write anymore *****/
   Gbl.Layout.DivsEndWritten = Gbl.Layout.HTMLEndWritten = true;
  }

void TL_FavNoteUsr (void)
  {
   struct TL_Note SocNot;

   /***** Get user whom profile is displayed *****/
   Usr_GetParamOtherUsrCodEncryptedAndGetUsrData ();

   /***** Mark note as favourite *****/
   TL_FavNote (&SocNot);

   /***** Write HTML inside DIV with form to unfav *****/
   TL_PutFormToUnfavNote (&SocNot);

   /***** All the output is made, so don't write anymore *****/
   Gbl.Layout.DivsEndWritten = Gbl.Layout.HTMLEndWritten = true;
  }

static void TL_FavNote (struct TL_Note *SocNot)
  {
   bool ItsMe;
   long OriginalPubCod;

   /***** Get data of note *****/
   SocNot->NotCod = TL_GetParamNotCod ();
   TL_GetDataOfNoteByCod (SocNot);

   if (SocNot->NotCod > 0)
     {
      ItsMe = Usr_ItsMe (SocNot->UsrCod);
      if (Gbl.Usrs.Me.Logged && !ItsMe)	// I am not the author
	 if (!TL_CheckIfNoteIsFavedByUsr (SocNot->NotCod,
					   Gbl.Usrs.Me.UsrDat.UsrCod))	// I have not yet favourited the note
	   {
	    /***** Mark as favourite in database *****/
	    DB_QueryINSERT ("can not favourite note",
			    "INSERT IGNORE INTO social_notes_fav"
			    " (NotCod,UsrCod,TimeFav)"
			    " VALUES"
			    " (%ld,%ld,NOW())",
			    SocNot->NotCod,
			    Gbl.Usrs.Me.UsrDat.UsrCod);

	    /***** Update number of times this note is favourited *****/
	    TL_GetNumTimesANoteHasBeenFav (SocNot);

	    /***** Create notification about favourite post
		   for the author of the post *****/
	    OriginalPubCod = TL_GetPubCodOfOriginalNote (SocNot->NotCod);
	    if (OriginalPubCod > 0)
	       TL_CreateNotifToAuthor (SocNot->UsrCod,OriginalPubCod,Ntf_EVENT_TIMELINE_FAV);
	   }
     }
  }

/*****************************************************************************/
/************************ Mark a comment as favourite ************************/
/*****************************************************************************/

void TL_FavCommentGbl (void)
  {
   struct TL_Comment SocCom;

   /***** Mark comment as favourite *****/
   TL_FavComment (&SocCom);

   /***** Write HTML inside DIV with form to unfav *****/
   TL_PutFormToUnfavComment (&SocCom);

   /***** All the output is made, so don't write anymore *****/
   Gbl.Layout.DivsEndWritten = Gbl.Layout.HTMLEndWritten = true;
  }

void TL_FavCommentUsr (void)
  {
   struct TL_Comment SocCom;

   /***** Get user whom profile is displayed *****/
   Usr_GetParamOtherUsrCodEncryptedAndGetUsrData ();

   /***** Mark comment as favourite *****/
   TL_FavComment (&SocCom);

   /***** Write HTML inside DIV with form to unfav *****/
   TL_PutFormToUnfavComment (&SocCom);

   /***** All the output is made, so don't write anymore *****/
   Gbl.Layout.DivsEndWritten = Gbl.Layout.HTMLEndWritten = true;
  }

static void TL_FavComment (struct TL_Comment *SocCom)
  {
   bool ItsMe;

   /***** Initialize image *****/
   Med_MediaConstructor (&SocCom->Media);

   /***** Get data of comment *****/
   SocCom->PubCod = TL_GetParamPubCod ();
   TL_GetDataOfCommByCod (SocCom);

   if (SocCom->PubCod > 0)
     {
      ItsMe = Usr_ItsMe (SocCom->UsrCod);
      if (Gbl.Usrs.Me.Logged && !ItsMe)	// I am not the author
	 if (!TL_CheckIfCommIsFavedByUsr (SocCom->PubCod,
					   Gbl.Usrs.Me.UsrDat.UsrCod)) // I have not yet favourited the comment
	   {
	    /***** Mark as favourite in database *****/
	    DB_QueryINSERT ("can not favourite comment",
			    "INSERT IGNORE INTO social_comments_fav"
			    " (PubCod,UsrCod,TimeFav)"
			    " VALUES"
			    " (%ld,%ld,NOW())",
			    SocCom->PubCod,
			    Gbl.Usrs.Me.UsrDat.UsrCod);

	    /* Update number of times this comment is favourited */
	    TL_GetNumTimesACommHasBeenFav (SocCom);

	    /**** Create notification about favourite post
		  for the author of the post ***/
	    TL_CreateNotifToAuthor (SocCom->UsrCod,SocCom->PubCod,Ntf_EVENT_TIMELINE_FAV);
	   }
     }

   /***** Free image *****/
   Med_MediaDestructor (&SocCom->Media);
  }

/*****************************************************************************/
/*********** Create a notification for the author of a post/comment **********/
/*****************************************************************************/

static void TL_CreateNotifToAuthor (long AuthorCod,long PubCod,
                                    Ntf_NotifyEvent_t NotifyEvent)
  {
   struct UsrData UsrDat;
   bool CreateNotif;
   bool NotifyByEmail;

   /***** Initialize structure with user's data *****/
   Usr_UsrDataConstructor (&UsrDat);

   UsrDat.UsrCod = AuthorCod;
   if (Usr_ChkUsrCodAndGetAllUsrDataFromUsrCod (&UsrDat))
     {
      /***** This fav must be notified by email? *****/
      CreateNotif = (UsrDat.Prefs.NotifNtfEvents & (1 << NotifyEvent));
      NotifyByEmail = CreateNotif &&
		      (UsrDat.Prefs.EmailNtfEvents & (1 << NotifyEvent));

      /***** Create notification for the author of the post.
	     If this author wants to receive notifications by email,
	     activate the sending of a notification *****/
      if (CreateNotif)
	 Ntf_StoreNotifyEventToOneUser (NotifyEvent,&UsrDat,PubCod,
					(Ntf_Status_t) (NotifyByEmail ? Ntf_STATUS_BIT_EMAIL :
									0));
     }

   /***** Free memory used for user's data *****/
   Usr_UsrDataDestructor (&UsrDat);
  }

/*****************************************************************************/
/******************** Unshare a previously shared note ***********************/
/*****************************************************************************/

void TL_UnshareNoteGbl (void)
  {
   struct TL_Note SocNot;

   /***** Unshare note *****/
   TL_UnshareNote (&SocNot);

   /***** Write HTML inside DIV with form to share *****/
   TL_PutFormToShareNote (&SocNot);

   /***** All the output is made, so don't write anymore *****/
   Gbl.Layout.DivsEndWritten = Gbl.Layout.HTMLEndWritten = true;
  }

void TL_UnshareNoteUsr (void)
  {
   struct TL_Note SocNot;

   /***** Get user whom profile is displayed *****/
   Usr_GetParamOtherUsrCodEncryptedAndGetUsrData ();

   /***** Share note *****/
   TL_UnshareNote (&SocNot);

   /***** Write HTML inside DIV with form to share *****/
   TL_PutFormToShareNote (&SocNot);

   /***** All the output is made, so don't write anymore *****/
   Gbl.Layout.DivsEndWritten = Gbl.Layout.HTMLEndWritten = true;
  }

static void TL_UnshareNote (struct TL_Note *SocNot)
  {
   extern const char *Txt_The_original_post_no_longer_exists;
   long OriginalPubCod;
   bool ItsMe;

   /***** Get data of note *****/
   SocNot->NotCod = TL_GetParamNotCod ();
   TL_GetDataOfNoteByCod (SocNot);

   if (SocNot->NotCod > 0)
     {
      ItsMe = Usr_ItsMe (SocNot->UsrCod);
      if (SocNot->NumShared &&
	  Gbl.Usrs.Me.Logged && !ItsMe)	// I am not the author
	 if (TL_CheckIfNoteIsSharedByUsr (SocNot->NotCod,
					   Gbl.Usrs.Me.UsrDat.UsrCod))	// I am a sharer
	   {
	    /***** Delete publication from database *****/
	    DB_QueryDELETE ("can not remove a publication",
			    "DELETE FROM social_pubs"
	                    " WHERE NotCod=%ld"
	                    " AND PublisherCod=%ld"
	                    " AND PubType=%u",
			    SocNot->NotCod,
			    Gbl.Usrs.Me.UsrDat.UsrCod,
			    (unsigned) TL_PUB_SHARED_NOTE);

	    /***** Update number of times this note is shared *****/
	    TL_UpdateNumTimesANoteHasBeenShared (SocNot);

            /***** Mark possible notifications on this note as removed *****/
	    OriginalPubCod = TL_GetPubCodOfOriginalNote (SocNot->NotCod);
	    if (OriginalPubCod > 0)
	       Ntf_MarkNotifAsRemoved (Ntf_EVENT_TIMELINE_SHARE,OriginalPubCod);
	   }
     }
  }

/*****************************************************************************/
/*********** Stop marking as favourite a previously favourited note **********/
/*****************************************************************************/

void TL_UnfavNoteGbl (void)
  {
   struct TL_Note SocNot;

   /***** Stop marking as favourite a previously favourited note *****/
   TL_UnfavNote (&SocNot);

   /***** Write HTML inside DIV with form to fav *****/
   TL_PutFormToFavNote (&SocNot);

   /***** All the output is made, so don't write anymore *****/
   Gbl.Layout.DivsEndWritten = Gbl.Layout.HTMLEndWritten = true;
  }

void TL_UnfavNoteUsr (void)
  {
   struct TL_Note SocNot;

   /***** Get user whom profile is displayed *****/
   Usr_GetParamOtherUsrCodEncryptedAndGetUsrData ();

   /***** Unfav a note previously marked as favourite *****/
   TL_UnfavNote (&SocNot);

   /***** Write HTML inside DIV with form to fav *****/
   TL_PutFormToFavNote (&SocNot);

   /***** All the output is made, so don't write anymore *****/
   Gbl.Layout.DivsEndWritten = Gbl.Layout.HTMLEndWritten = true;
  }

static void TL_UnfavNote (struct TL_Note *SocNot)
  {
   long OriginalPubCod;
   bool ItsMe;

   /***** Get data of note *****/
   SocNot->NotCod = TL_GetParamNotCod ();
   TL_GetDataOfNoteByCod (SocNot);

   if (SocNot->NotCod > 0)
     {
      ItsMe = Usr_ItsMe (SocNot->UsrCod);
      if (SocNot->NumFavs &&
	  Gbl.Usrs.Me.Logged && !ItsMe)	// I am not the author
	 if (TL_CheckIfNoteIsFavedByUsr (SocNot->NotCod,
					  Gbl.Usrs.Me.UsrDat.UsrCod))	// I have favourited the note
	   {
	    /***** Delete the mark as favourite from database *****/
	    DB_QueryDELETE ("can not unfavourite note",
			    "DELETE FROM social_notes_fav"
			    " WHERE NotCod=%ld AND UsrCod=%ld",
			    SocNot->NotCod,
			    Gbl.Usrs.Me.UsrDat.UsrCod);

	    /***** Update number of times this note is favourited *****/
	    TL_GetNumTimesANoteHasBeenFav (SocNot);

            /***** Mark possible notifications on this note as removed *****/
	    OriginalPubCod = TL_GetPubCodOfOriginalNote (SocNot->NotCod);
	    if (OriginalPubCod > 0)
	       Ntf_MarkNotifAsRemoved (Ntf_EVENT_TIMELINE_FAV,OriginalPubCod);
	   }
     }
  }

/*****************************************************************************/
/********* Stop marking as favourite a previously favourited comment *********/
/*****************************************************************************/

void TL_UnfavCommentGbl (void)
  {
   struct TL_Comment SocCom;

   /***** Stop marking as favourite a previously favourited comment *****/
   TL_UnfavComment (&SocCom);

   /***** Write HTML inside DIV with form to fav *****/
   TL_PutFormToFavComment (&SocCom);

   /***** All the output is made, so don't write anymore *****/
   Gbl.Layout.DivsEndWritten = Gbl.Layout.HTMLEndWritten = true;
  }

void TL_UnfavCommentUsr (void)
  {
   struct TL_Comment SocCom;

   /***** Get user whom profile is displayed *****/
   Usr_GetParamOtherUsrCodEncryptedAndGetUsrData ();

   /***** Unfav a comment previously marked as favourite *****/
   TL_UnfavComment (&SocCom);

   /***** Write HTML inside DIV with form to fav *****/
   TL_PutFormToFavComment (&SocCom);

   /***** All the output is made, so don't write anymore *****/
   Gbl.Layout.DivsEndWritten = Gbl.Layout.HTMLEndWritten = true;
  }

static void TL_UnfavComment (struct TL_Comment *SocCom)
  {
   bool ItsMe;

   /***** Initialize image *****/
   Med_MediaConstructor (&SocCom->Media);

   /***** Get data of comment *****/
   SocCom->PubCod = TL_GetParamPubCod ();
   TL_GetDataOfCommByCod (SocCom);

   if (SocCom->PubCod > 0)
     {
      ItsMe = Usr_ItsMe (SocCom->UsrCod);
      if (SocCom->NumFavs &&
	  Gbl.Usrs.Me.Logged && !ItsMe)	// I am not the author
	 if (TL_CheckIfCommIsFavedByUsr (SocCom->PubCod,
					  Gbl.Usrs.Me.UsrDat.UsrCod))	// I have favourited the comment
	   {
	    /***** Delete the mark as favourite from database *****/
	    DB_QueryDELETE ("can not unfavourite comment",
			    "DELETE FROM social_comments_fav"
			    " WHERE PubCod=%ld AND UsrCod=%ld",
			    SocCom->PubCod,
			    Gbl.Usrs.Me.UsrDat.UsrCod);

	    /***** Update number of times this comment is favourited *****/
	    TL_GetNumTimesACommHasBeenFav (SocCom);

            /***** Mark possible notifications on this comment as removed *****/
            Ntf_MarkNotifAsRemoved (Ntf_EVENT_TIMELINE_FAV,SocCom->PubCod);
	   }
     }

   /***** Free image *****/
   Med_MediaDestructor (&SocCom->Media);
  }

/*****************************************************************************/
/*********************** Request the removal of a note ***********************/
/*****************************************************************************/

void TL_RequestRemNoteGbl (void)
  {
   /***** Request the removal of note *****/
   TL_RequestRemovalNote ();

   /***** Write timeline again (global) *****/
   TL_ShowTimelineGbl2 ();
  }

void TL_RequestRemNoteUsr (void)
  {
   /***** Get user whom profile is displayed *****/
   Usr_GetParamOtherUsrCodEncryptedAndGetUsrData ();

   /***** Show user's profile *****/
   Prf_ShowUserProfile (&Gbl.Usrs.Other.UsrDat);

   /***** Start section *****/
   Lay_StartSection (TL_TIMELINE_SECTION_ID);

   /***** Request the removal of note *****/
   TL_RequestRemovalNote ();

   /***** Write timeline again (user) *****/
   TL_ShowTimelineUsr ();

   /***** End section *****/
   Lay_EndSection ();
  }

static void TL_RequestRemovalNote (void)
  {
   extern const char *Txt_The_original_post_no_longer_exists;
   extern const char *Txt_Do_you_really_want_to_remove_the_following_post;
   extern const char *Txt_Remove;
   struct TL_Note SocNot;
   bool ItsMe;

   /***** Get data of note *****/
   SocNot.NotCod = TL_GetParamNotCod ();
   TL_GetDataOfNoteByCod (&SocNot);

   if (SocNot.NotCod > 0)
     {
      ItsMe = Usr_ItsMe (SocNot.UsrCod);
      if (ItsMe)	// I am the author of this note
	{
	 /***** Show question and button to remove note *****/
	 /* Start alert */
	 Ale_ShowAlertAndButton1 (Ale_QUESTION,Txt_Do_you_really_want_to_remove_the_following_post);

	 /* Show note */
	 TL_WriteNote (&SocNot,
		       TL_TOP_MESSAGE_NONE,-1L,
		       false,true);

	 /* End alert */
	 Gbl.Timeline.NotCod = SocNot.NotCod;	// Note to be removed
	 if (Gbl.Usrs.Other.UsrDat.UsrCod > 0)
	    Ale_ShowAlertAndButton2 (ActRemSocPubUsr,"timeline",NULL,
	                             TL_PutParamsRemoveNote,
				     Btn_REMOVE_BUTTON,Txt_Remove);
	 else
	    Ale_ShowAlertAndButton2 (ActRemSocPubGbl,NULL,NULL,
	                             TL_PutParamsRemoveNote,
				     Btn_REMOVE_BUTTON,Txt_Remove);
	}
     }
   else
      Ale_ShowAlert (Ale_WARNING,Txt_The_original_post_no_longer_exists);
  }

/*****************************************************************************/
/********************* Put parameters to remove a note ***********************/
/*****************************************************************************/

static void TL_PutParamsRemoveNote (void)
  {
   if (Gbl.Usrs.Other.UsrDat.UsrCod > 0)
      Usr_PutParamOtherUsrCodEncrypted ();
   else
      TL_PutParamWhichUsrs ();
   TL_PutHiddenParamNotCod (Gbl.Timeline.NotCod);
  }

/*****************************************************************************/
/******************************* Remove a note *******************************/
/*****************************************************************************/

void TL_RemoveNoteGbl (void)
  {
   /***** Remove a note *****/
   TL_RemoveNote ();

   /***** Write updated timeline after removing (global) *****/
   TL_ShowTimelineGbl2 ();
  }

void TL_RemoveNoteUsr (void)
  {
   /***** Get user whom profile is displayed *****/
   Usr_GetParamOtherUsrCodEncryptedAndGetUsrData ();

   /***** Show user's profile *****/
   Prf_ShowUserProfile (&Gbl.Usrs.Other.UsrDat);

   /***** Start section *****/
   Lay_StartSection (TL_TIMELINE_SECTION_ID);

   /***** Remove a note *****/
   TL_RemoveNote ();

   /***** Write updated timeline after removing (user) *****/
   TL_ShowTimelineUsr ();

   /***** End section *****/
   Lay_EndSection ();
  }

static void TL_RemoveNote (void)
  {
   extern const char *Txt_The_original_post_no_longer_exists;
   extern const char *Txt_TIMELINE_Post_removed;
   struct TL_Note SocNot;
   bool ItsMe;

   /***** Get data of note *****/
   SocNot.NotCod = TL_GetParamNotCod ();
   TL_GetDataOfNoteByCod (&SocNot);

   if (SocNot.NotCod > 0)
     {
      ItsMe = Usr_ItsMe (SocNot.UsrCod);
      if (ItsMe)	// I am the author of this note
	{
	 if (SocNot.NoteType == TL_NOTE_POST)
            /***** Remove image file associated to post *****/
            TL_RemoveImgFileFromPost (SocNot.Cod);

	 /***** Delete note from database *****/
	 TL_RemoveANoteFromDB (&SocNot);

	 /***** Message of success *****/
	 Ale_ShowAlert (Ale_SUCCESS,Txt_TIMELINE_Post_removed);
	}
     }
   else
      Ale_ShowAlert (Ale_WARNING,Txt_The_original_post_no_longer_exists);
  }

/*****************************************************************************/
/***************** Remove one file associated to a post **********************/
/*****************************************************************************/

static void TL_RemoveImgFileFromPost (long PstCod)
  {
   MYSQL_RES *mysql_res;

   /***** Get name of image associated to a post from database *****/
   if (DB_QuerySELECT (&mysql_res,"can not get image",
		       "SELECT MediaName,"	// row[0]
		              "MediaType"	// row[1]
		       " FROM social_posts"
		       " WHERE PstCod=%ld",
		       PstCod))
      /***** Remove media file *****/
      Med_RemoveMediaFilesFromRow (mysql_res);

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/*********************** Remove a note from database *************************/
/*****************************************************************************/

static void TL_RemoveANoteFromDB (struct TL_Note *SocNot)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   long PubCod;
   unsigned long NumComments;
   unsigned long NumCom;

   /***** Mark possible notifications on the publications
          of this note as removed *****/
   /* Mark notifications of the original note as removed */
   PubCod = TL_GetPubCodOfOriginalNote (SocNot->NotCod);
   if (PubCod > 0)
     {
      Ntf_MarkNotifAsRemoved (Ntf_EVENT_TIMELINE_COMMENT,PubCod);
      Ntf_MarkNotifAsRemoved (Ntf_EVENT_TIMELINE_FAV    ,PubCod);
      Ntf_MarkNotifAsRemoved (Ntf_EVENT_TIMELINE_SHARE  ,PubCod);
      Ntf_MarkNotifAsRemoved (Ntf_EVENT_TIMELINE_MENTION,PubCod);
     }

   /* Get comments of this note */
   NumComments = DB_QuerySELECT (&mysql_res,"can not get comments",
				 "SELECT PubCod"
				 " FROM social_pubs"
				 " WHERE NotCod=%ld AND PubType=%u",
				 SocNot->NotCod,
				 (unsigned) TL_PUB_COMMENT_TO_NOTE);

   /* For each comment... */
   for (NumCom = 0;
	NumCom < NumComments;
	NumCom++)
     {
      /* Get code of comment **/
      row = mysql_fetch_row (mysql_res);
      PubCod = Str_ConvertStrCodToLongCod (row[0]);

      /* Mark notifications as removed */
      if (PubCod > 0)
	{
	 Ntf_MarkNotifAsRemoved (Ntf_EVENT_TIMELINE_COMMENT,PubCod);
	 Ntf_MarkNotifAsRemoved (Ntf_EVENT_TIMELINE_FAV    ,PubCod);
	 Ntf_MarkNotifAsRemoved (Ntf_EVENT_TIMELINE_MENTION,PubCod);
	}
     }

   /* Free structure that stores the query result */
   DB_FreeMySQLResult (&mysql_res);

   /***** Remove favs *****/
   /* Remove favs for all comments in this note */
   DB_QueryDELETE ("can not remove favs for note",
		   "DELETE FROM social_comments_fav"
                   " USING social_pubs,social_comments_fav"
	           " WHERE social_pubs.NotCod=%ld"
                   " AND social_pubs.PubType=%u"
	           " AND social_pubs.PubCod=social_comments_fav.PubCod",
		   SocNot->NotCod,(unsigned) TL_PUB_COMMENT_TO_NOTE);

   /* Remove favs for this note */
   DB_QueryDELETE ("can not remove favs for note",
		   "DELETE FROM social_notes_fav"
		   " WHERE NotCod=%ld",
		   SocNot->NotCod);

   /***** Remove content of the comments of this note *****/
   DB_QueryDELETE ("can not remove comments",
		   "DELETE FROM social_comments"
	           " USING social_pubs,social_comments"
	           " WHERE social_pubs.NotCod=%ld"
                   " AND social_pubs.PubType=%u"
	           " AND social_pubs.PubCod=social_comments.PubCod",
		   SocNot->NotCod,(unsigned) TL_PUB_COMMENT_TO_NOTE);

   /***** Remove all the publications of this note *****/
   DB_QueryDELETE ("can not remove a publication",
		   "DELETE FROM social_pubs"
		   " WHERE NotCod=%ld",
		   SocNot->NotCod);

   /***** Remove note *****/
   DB_QueryDELETE ("can not remove a note",
		   "DELETE FROM social_notes"
	           " WHERE NotCod=%ld"
	           " AND UsrCod=%ld",		// Extra check: I am the author
		   SocNot->NotCod,
		   Gbl.Usrs.Me.UsrDat.UsrCod);

   if (SocNot->NoteType == TL_NOTE_POST)
      /***** Remove post *****/
      DB_QueryDELETE ("can not remove a post",
		      "DELETE FROM social_posts"
		      " WHERE PstCod=%ld",
		      SocNot->Cod);

   /***** Reset note *****/
   TL_ResetNote (SocNot);
  }

/*****************************************************************************/
/*********************** Get code of note of a publication *******************/
/*****************************************************************************/

static long TL_GetNotCodOfPublication (long PubCod)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   long NotCod = -1L;

   /***** Get code of note from database *****/
   if (DB_QuerySELECT (&mysql_res,"can not get code of note",
		       "SELECT NotCod FROM social_pubs"
		       " WHERE PubCod=%ld",
		       PubCod) == 1)   // Result should have a unique row
     {
      /* Get code of note */
      row = mysql_fetch_row (mysql_res);
      NotCod = Str_ConvertStrCodToLongCod (row[0]);
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   return NotCod;
  }

/*****************************************************************************/
/*************** Get code of publication of the original note ****************/
/*****************************************************************************/

static long TL_GetPubCodOfOriginalNote (long NotCod)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   long OriginalPubCod = -1L;

   /***** Get code of publication of the original note *****/
   if (DB_QuerySELECT (&mysql_res,"can not get code of publication",
		       "SELECT PubCod FROM social_pubs"
		       " WHERE NotCod=%ld AND PubType=%u",
		       NotCod,(unsigned) TL_PUB_ORIGINAL_NOTE) == 1)   // Result should have a unique row
     {
      /* Get code of publication (row[0]) */
      row = mysql_fetch_row (mysql_res);
      OriginalPubCod = Str_ConvertStrCodToLongCod (row[0]);
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   return OriginalPubCod;
  }

/*****************************************************************************/
/**************** Request the removal of a comment in a note *****************/
/*****************************************************************************/

void TL_RequestRemComGbl (void)
  {
   /***** Request the removal of comment in note *****/
   TL_RequestRemovalComment ();

   /***** Write timeline again (global) *****/
   TL_ShowTimelineGbl2 ();
  }

void TL_RequestRemComUsr (void)
  {
   /***** Get user whom profile is displayed *****/
   Usr_GetParamOtherUsrCodEncryptedAndGetUsrData ();

   /***** Show user's profile *****/
   Prf_ShowUserProfile (&Gbl.Usrs.Other.UsrDat);

   /***** Start section *****/
   Lay_StartSection (TL_TIMELINE_SECTION_ID);

   /***** Request the removal of comment in note *****/
   TL_RequestRemovalComment ();

   /***** Write timeline again (user) *****/
   TL_ShowTimelineUsr ();

   /***** End section *****/
   Lay_EndSection ();
  }

static void TL_RequestRemovalComment (void)
  {
   extern const char *Txt_The_comment_no_longer_exists;
   extern const char *Txt_Do_you_really_want_to_remove_the_following_comment;
   extern const char *Txt_Remove;
   struct TL_Comment SocCom;
   bool ItsMe;

   /***** Initialize image *****/
   Med_MediaConstructor (&SocCom.Media);

   /***** Get data of comment *****/
   SocCom.PubCod = TL_GetParamPubCod ();
   TL_GetDataOfCommByCod (&SocCom);

   if (SocCom.PubCod > 0)
     {
      ItsMe = Usr_ItsMe (SocCom.UsrCod);
      if (ItsMe)	// I am the author of this comment
	{
	 /***** Show question and button to remove comment *****/
	 /* Start alert */
	 Ale_ShowAlertAndButton1 (Ale_QUESTION,Txt_Do_you_really_want_to_remove_the_following_comment);

	 /* Show comment */
	 TL_WriteComment (&SocCom,
				 TL_TOP_MESSAGE_NONE,-1L,
				 true);

	 /* End alert */
	 Gbl.Timeline.PubCod = SocCom.PubCod;	// Publication to be removed
	 if (Gbl.Usrs.Other.UsrDat.UsrCod > 0)
	    Ale_ShowAlertAndButton2 (ActRemSocComUsr,"timeline",NULL,
	                             TL_PutParamsRemoveCommment,
				     Btn_REMOVE_BUTTON,Txt_Remove);
	 else
	    Ale_ShowAlertAndButton2 (ActRemSocComGbl,NULL,NULL,
	                             TL_PutParamsRemoveCommment,
				     Btn_REMOVE_BUTTON,Txt_Remove);
	}
     }
   else
      Ale_ShowAlert (Ale_WARNING,Txt_The_comment_no_longer_exists);

   /***** Free image *****/
   Med_MediaDestructor (&SocCom.Media);
  }

/*****************************************************************************/
/******************** Put parameters to remove a comment *********************/
/*****************************************************************************/

static void TL_PutParamsRemoveCommment (void)
  {
   if (Gbl.Usrs.Other.UsrDat.UsrCod > 0)
      Usr_PutParamOtherUsrCodEncrypted ();
   else
      TL_PutParamWhichUsrs ();
   TL_PutHiddenParamPubCod (Gbl.Timeline.PubCod);
  }

/*****************************************************************************/
/***************************** Remove a comment ******************************/
/*****************************************************************************/

void TL_RemoveComGbl (void)
  {
   /***** Remove a comment *****/
   TL_RemoveComment ();

   /***** Write updated timeline after removing (global) *****/
   TL_ShowTimelineGbl2 ();
  }

void TL_RemoveComUsr (void)
  {
   /***** Get user whom profile is displayed *****/
   Usr_GetParamOtherUsrCodEncryptedAndGetUsrData ();

   /***** Show user's profile *****/
   Prf_ShowUserProfile (&Gbl.Usrs.Other.UsrDat);

   /***** Start section *****/
   Lay_StartSection (TL_TIMELINE_SECTION_ID);

   /***** Remove a comment *****/
   TL_RemoveComment ();

   /***** Write updated timeline after removing (user) *****/
   TL_ShowTimelineUsr ();

   /***** End section *****/
   Lay_EndSection ();
  }

static void TL_RemoveComment (void)
  {
   extern const char *Txt_The_comment_no_longer_exists;
   extern const char *Txt_Comment_removed;
   struct TL_Comment SocCom;
   bool ItsMe;

   /***** Initialize image *****/
   Med_MediaConstructor (&SocCom.Media);

   /***** Get data of comment *****/
   SocCom.PubCod = TL_GetParamPubCod ();
   TL_GetDataOfCommByCod (&SocCom);

   if (SocCom.PubCod > 0)
     {
      ItsMe = Usr_ItsMe (SocCom.UsrCod);
      if (ItsMe)	// I am the author of this comment
	{
	 /***** Remove image file associated to post *****/
	 TL_RemoveImgFileFromComment (SocCom.PubCod);

	 /***** Delete comment from database *****/
	 TL_RemoveACommentFromDB (&SocCom);

	 /***** Message of success *****/
	 Ale_ShowAlert (Ale_SUCCESS,Txt_Comment_removed);
	}
     }
   else
      Ale_ShowAlert (Ale_WARNING,Txt_The_comment_no_longer_exists);

   /***** Free image *****/
   Med_MediaDestructor (&SocCom.Media);
  }

/*****************************************************************************/
/**************** Remove one file associated to a comment ********************/
/*****************************************************************************/

static void TL_RemoveImgFileFromComment (long PubCod)
  {
   MYSQL_RES *mysql_res;

   /***** Get name of media associated to a post from database *****/
   if (DB_QuerySELECT (&mysql_res,"can not get media",
		       "SELECT MediaName,"	// row[0]
		              "MediaType"	// row[1]
		       " FROM social_comments"
		       " WHERE PubCod=%ld",
		       PubCod))
      /***** Remove media file *****/
      Med_RemoveMediaFilesFromRow (mysql_res);

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/********************* Remove a comment from database ************************/
/*****************************************************************************/

static void TL_RemoveACommentFromDB (struct TL_Comment *SocCom)
  {
   /***** Mark possible notifications on this comment as removed *****/
   Ntf_MarkNotifAsRemoved (Ntf_EVENT_TIMELINE_COMMENT,SocCom->PubCod);
   Ntf_MarkNotifAsRemoved (Ntf_EVENT_TIMELINE_FAV    ,SocCom->PubCod);
   Ntf_MarkNotifAsRemoved (Ntf_EVENT_TIMELINE_MENTION,SocCom->PubCod);

   /***** Remove favs for this comment *****/
   DB_QueryDELETE ("can not remove favs for comment",
		   "DELETE FROM social_comments_fav"
		   " WHERE PubCod=%ld",
		   SocCom->PubCod);

   /***** Remove content of this comment *****/
   DB_QueryDELETE ("can not remove a comment",
		   "DELETE FROM social_comments"
		   " WHERE PubCod=%ld",
		   SocCom->PubCod);

   /***** Remove this comment *****/
   DB_QueryDELETE ("can not remove a comment",
		   "DELETE FROM social_pubs"
	           " WHERE PubCod=%ld"
	           " AND PublisherCod=%ld"	// Extra check: I am the author
	           " AND PubType=%u",		// Extra check: it's a comment
		   SocCom->PubCod,
		   Gbl.Usrs.Me.UsrDat.UsrCod,
		   (unsigned) TL_PUB_COMMENT_TO_NOTE);

   /***** Reset comment *****/
   TL_ResetComment (SocCom);
  }

/*****************************************************************************/
/************* Remove all the content of a user from database ****************/
/*****************************************************************************/

void TL_RemoveUsrContent (long UsrCod)
  {
   /***** Remove favs for comments *****/
   /* Remove all favs made by this user in any comment */
   DB_QueryDELETE ("can not remove favs",
		   "DELETE FROM social_comments_fav"
		   " WHERE UsrCod=%ld",
		   UsrCod);

   /* Remove all favs for all comments of this user */
   DB_QueryDELETE ("can not remove favs",
		   "DELETE FROM social_comments_fav"
	           " USING social_pubs,social_comments_fav"
	           " WHERE social_pubs.PublisherCod=%ld"	// Author of the comment
                   " AND social_pubs.PubType=%u"
	           " AND social_pubs.PubCod=social_comments_fav.PubCod",
		   UsrCod,(unsigned) TL_PUB_COMMENT_TO_NOTE);

   /* Remove all favs for all comments in all the notes of the user */
   DB_QueryDELETE ("can not remove comments",
		   "DELETE FROM social_comments_fav"
	           " USING social_notes,social_pubs,social_comments_fav"
	           " WHERE social_notes.UsrCod=%ld"	// Author of the note
	           " AND social_notes.NotCod=social_pubs.NotCod"
                   " AND social_pubs.PubType=%u"
	           " AND social_pubs.PubCod=social_comments_fav.PubCod",
		   UsrCod,(unsigned) TL_PUB_COMMENT_TO_NOTE);

   /***** Remove favs for notes *****/
   /* Remove all favs made by this user in any note */
   DB_QueryDELETE ("can not remove favs",
		   "DELETE FROM social_notes_fav"
		   " WHERE UsrCod=%ld",
		   UsrCod);

   /* Remove all favs for all notes of this user */
   DB_QueryDELETE ("can not remove favs",
		   "DELETE FROM social_notes_fav"
	           " USING social_notes,social_notes_fav"
	           " WHERE social_notes.UsrCod=%ld"	// Author of the note
	           " AND social_notes.NotCod=social_notes_fav.NotCod",
		   UsrCod);

   /***** Remove comments *****/
   /* Remove content of all the comments in all the notes of the user */
   DB_QueryDELETE ("can not remove comments",
		   "DELETE FROM social_comments"
	           " USING social_notes,social_pubs,social_comments"
	           " WHERE social_notes.UsrCod=%ld"
		   " AND social_notes.NotCod=social_pubs.NotCod"
                   " AND social_pubs.PubType=%u"
	           " AND social_pubs.PubCod=social_comments.PubCod",
		   UsrCod,(unsigned) TL_PUB_COMMENT_TO_NOTE);

   /* Remove all the comments from any user in any note of the user */
   DB_QueryDELETE ("can not remove comments",
		   "DELETE FROM social_pubs"
	           " USING social_notes,social_pubs"
	           " WHERE social_notes.UsrCod=%ld"
		   " AND social_notes.NotCod=social_pubs.NotCod"
                   " AND social_pubs.PubType=%u",
		   UsrCod,(unsigned) TL_PUB_COMMENT_TO_NOTE);

   /* Remove content of all the comments of the user in any note */
   DB_QueryDELETE ("can not remove comments",
		   "DELETE FROM social_comments"
	           " USING social_pubs,social_comments"
	           " WHERE social_pubs.PublisherCod=%ld"
	           " AND social_pubs.PubType=%u"
	           " AND social_pubs.PubCod=social_comments.PubCod",
		   UsrCod,(unsigned) TL_PUB_COMMENT_TO_NOTE);

   /***** Remove all the posts of the user *****/
   DB_QueryDELETE ("can not remove posts",
		   "DELETE FROM social_posts"
		   " WHERE PstCod IN"
		   " (SELECT Cod FROM social_notes"
	           " WHERE UsrCod=%ld AND NoteType=%u)",
		   UsrCod,(unsigned) TL_NOTE_POST);

   /***** Remove all the publications of any user authored by the user *****/
   DB_QueryDELETE ("can not remove publications",
		   "DELETE FROM social_pubs"
                   " USING social_notes,social_pubs"
	           " WHERE social_notes.UsrCod=%ld"
                   " AND social_notes.NotCod=social_pubs.NotCod",
		   UsrCod);

   /***** Remove all the publications of the user *****/
   DB_QueryDELETE ("can not remove publications",
		   "DELETE FROM social_pubs"
		   " WHERE PublisherCod=%ld",
		   UsrCod);

   /***** Remove all the notes of the user *****/
   DB_QueryDELETE ("can not remove notes",
		   "DELETE FROM social_notes"
		   " WHERE UsrCod=%ld",
		   UsrCod);
  }

/*****************************************************************************/
/****************** Check if a user has published a note *********************/
/*****************************************************************************/

static bool TL_CheckIfNoteIsSharedByUsr (long NotCod,long UsrCod)
  {
   return (DB_QueryCOUNT ("can not check if a user has shared a note",
			  "SELECT COUNT(*) FROM social_pubs"
			  " WHERE NotCod=%ld"
			  " AND PublisherCod=%ld"
			  " AND PubType=%u",
			  NotCod,
			  UsrCod,
			  (unsigned) TL_PUB_SHARED_NOTE) != 0);
  }

/*****************************************************************************/
/****************** Check if a user has favourited a note ********************/
/*****************************************************************************/

static bool TL_CheckIfNoteIsFavedByUsr (long NotCod,long UsrCod)
  {
   return (DB_QueryCOUNT ("can not check if a user"
			  " has favourited a note",
			  "SELECT COUNT(*) FROM social_notes_fav"
			  " WHERE NotCod=%ld AND UsrCod=%ld",
			  NotCod,UsrCod) != 0);
  }

/*****************************************************************************/
/**************** Check if a user has favourited a comment *******************/
/*****************************************************************************/

static bool TL_CheckIfCommIsFavedByUsr (long PubCod,long UsrCod)
  {
   return (DB_QueryCOUNT ("can not check if a user"
			  " has favourited a comment",
			  "SELECT COUNT(*) FROM social_comments_fav"
			  " WHERE PubCod=%ld AND UsrCod=%ld",
			  PubCod,UsrCod) != 0);
  }

/*****************************************************************************/
/********** Get number of times a note has been shared in timeline ***********/
/*****************************************************************************/

static void TL_UpdateNumTimesANoteHasBeenShared (struct TL_Note *SocNot)
  {
   /***** Get number of times (users) this note has been shared *****/
   SocNot->NumShared =
   (unsigned) DB_QueryCOUNT ("can not get number of times"
			     " a note has been shared",
			     "SELECT COUNT(*) FROM social_pubs"
			     " WHERE NotCod=%ld"
			     " AND PublisherCod<>%ld"
			     " AND PubType=%u",
			     SocNot->NotCod,
			     SocNot->UsrCod,	// The author
			     (unsigned) TL_PUB_SHARED_NOTE);
  }

/*****************************************************************************/
/*************** Get number of times a note has been favourited **************/
/*****************************************************************************/

static void TL_GetNumTimesANoteHasBeenFav (struct TL_Note *SocNot)
  {
   /***** Get number of times (users) this note has been favourited *****/
   SocNot->NumFavs =
   (unsigned) DB_QueryCOUNT ("can not get number of times"
			     " a note has been favourited",
			     "SELECT COUNT(*) FROM social_notes_fav"
			     " WHERE NotCod=%ld"
			     " AND UsrCod<>%ld",	// Extra check
			     SocNot->NotCod,
			     SocNot->UsrCod);		// The author
  }

/*****************************************************************************/
/************ Get number of times a comment has been favourited **************/
/*****************************************************************************/

static void TL_GetNumTimesACommHasBeenFav (struct TL_Comment *SocCom)
  {
   /***** Get number of times (users) this comment has been favourited *****/
   SocCom->NumFavs =
   (unsigned) DB_QueryCOUNT ("can not get number of times"
			     " a comment has been favourited",
			     "SELECT COUNT(*) FROM social_comments_fav"
			     " WHERE PubCod=%ld"
			     " AND UsrCod<>%ld",	// Extra check
			     SocCom->PubCod,
			     SocCom->UsrCod);		// The author
  }

/*****************************************************************************/
/******************* Show users who have shared this note ********************/
/*****************************************************************************/

static void TL_ShowUsrsWhoHaveSharedNote (const struct TL_Note *SocNot)
  {
   MYSQL_RES *mysql_res;
   unsigned NumFirstUsrs = 0;

   /***** Get users who have shared this note *****/
   if (SocNot->NumShared)
      NumFirstUsrs =
      (unsigned) DB_QuerySELECT (&mysql_res,"can not get users",
				 "SELECT PublisherCod FROM social_pubs"
				 " WHERE NotCod=%ld"
				 " AND PublisherCod<>%ld"
				 " AND PubType=%u"
				 " ORDER BY PubCod LIMIT %u",
				 SocNot->NotCod,
				 SocNot->UsrCod,
				 (unsigned) TL_PUB_SHARED_NOTE,
				 TL_MAX_SHARERS_FAVERS_SHOWN);

   /***** Show users *****/
   TL_ShowSharersOrFavers (&mysql_res,SocNot->NumShared,NumFirstUsrs);

   /***** Free structure that stores the query result *****/
   if (SocNot->NumShared)
      DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/************ Show users who have marked this note as favourite **************/
/*****************************************************************************/

static void TL_ShowUsrsWhoHaveMarkedNoteAsFav (const struct TL_Note *SocNot)
  {
   MYSQL_RES *mysql_res;
   unsigned NumFirstUsrs = 0;

   /***** Get users who have marked this note as favourite *****/
   if (SocNot->NumFavs)
     {
      /***** Get list of users from database *****/
      NumFirstUsrs =
      (unsigned) DB_QuerySELECT (&mysql_res,"can not get users",
				 "SELECT UsrCod FROM social_notes_fav"
				 " WHERE NotCod=%ld"
				 " AND UsrCod<>%ld"	// Extra check
				 " ORDER BY FavCod LIMIT %u",
				 SocNot->NotCod,
				 SocNot->UsrCod,
				 TL_MAX_SHARERS_FAVERS_SHOWN);
     }

   /***** Show users *****/
   TL_ShowSharersOrFavers (&mysql_res,SocNot->NumFavs,NumFirstUsrs);

   /***** Free structure that stores the query result *****/
   if (SocNot->NumFavs)
      DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/************ Show users who have marked this note as favourite **************/
/*****************************************************************************/

static void TL_ShowUsrsWhoHaveMarkedCommAsFav (const struct TL_Comment *SocCom)
  {
   MYSQL_RES *mysql_res;
   unsigned NumFirstUsrs = 0;

   /***** Get users who have marked this comment as favourite *****/
   if (SocCom->NumFavs)
      /***** Get list of users from database *****/
      NumFirstUsrs =
      (unsigned) DB_QuerySELECT (&mysql_res,"can not get users",
				 "SELECT UsrCod FROM social_comments_fav"
				 " WHERE PubCod=%ld"
				 " AND UsrCod<>%ld"	// Extra check
				 " ORDER BY FavCod LIMIT %u",
				 SocCom->PubCod,
				 SocCom->UsrCod,
				 TL_MAX_SHARERS_FAVERS_SHOWN);

   /***** Show users *****/
   TL_ShowSharersOrFavers (&mysql_res,SocCom->NumFavs,NumFirstUsrs);

   /***** Free structure that stores the query result *****/
   if (SocCom->NumFavs)
      DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/************************ Show sharers or favouriters ************************/
/*****************************************************************************/
// All forms in this function and nested functions must have unique identifiers

static void TL_ShowSharersOrFavers (MYSQL_RES **mysql_res,
				    unsigned NumUsrs,unsigned NumFirstUsrs)
  {
   MYSQL_ROW row;
   unsigned NumUsr;
   unsigned NumUsrsShown = 0;
   struct UsrData UsrDat;
   bool ShowPhoto;
   char PhotoURL[PATH_MAX + 1];

   /***** Show number of users who have marked this note as favourite *****/
   fprintf (Gbl.F.Out,"<span class=\"TL_NUM_SHA_FAV\"> %u</span>",
            NumUsrs);

   if (NumUsrs)
     {
      /***** A list of users has been got from database *****/
      if (NumFirstUsrs)
	{
	 /***** Initialize structure with user's data *****/
	 Usr_UsrDataConstructor (&UsrDat);

	 /***** List users *****/
	 for (NumUsr = 0;
	      NumUsr < NumFirstUsrs;
	      NumUsr++)
	   {
	    /***** Get user *****/
	    row = mysql_fetch_row (*mysql_res);

	    /* Get user's code (row[0]) */
	    UsrDat.UsrCod = Str_ConvertStrCodToLongCod (row[0]);

	    /***** Get user's data and show user's photo *****/
	    if (Usr_ChkUsrCodAndGetAllUsrDataFromUsrCod (&UsrDat))
	      {
               fprintf (Gbl.F.Out,"<div class=\"TL_SHARER\">");
	       ShowPhoto = Pho_ShowingUsrPhotoIsAllowed (&UsrDat,PhotoURL);
	       Pho_ShowUsrPhoto (&UsrDat,ShowPhoto ? PhotoURL :
	                                             NULL,
	                         "PHOTO12x16",Pho_ZOOM,true);	// Use unique id
               fprintf (Gbl.F.Out,"</div>");

               NumUsrsShown++;
              }
	   }

	 /***** Free memory used for user's data *****/
	 Usr_UsrDataDestructor (&UsrDat);
	}

      if (NumUsrs > NumUsrsShown)
	 fprintf (Gbl.F.Out,"<div class=\"TL_SHARER\">"
	                    "<img src=\"%s/ellipsis-h.svg\""
			    " alt=\"%u\" title=\"%u\""
			    " class=\"ICO16x16\" />"
			    "</div>",
		  Gbl.Prefs.URLIcons,
		  NumUsrs - NumUsrsShown,
		  NumUsrs - NumUsrsShown);
     }
  }

/*****************************************************************************/
/******************** Get data of note using its code ************************/
/*****************************************************************************/

static void TL_GetDataOfNoteByCod (struct TL_Note *SocNot)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;

   if (SocNot->NotCod > 0)
     {
      /***** Get data of note from database *****/
      if (DB_QuerySELECT (&mysql_res,"can not get data of note",
			  "SELECT NotCod,"			// row[0]
				 "NoteType,"			// row[1]
				 "Cod,"				// row[2]
				 "UsrCod,"			// row[3]
				 "HieCod,"			// row[4]
				 "Unavailable,"			// row[5]
				 "UNIX_TIMESTAMP(TimeNote)"	// row[6]
			  " FROM social_notes"
			  " WHERE NotCod=%ld",
			  SocNot->NotCod))
	{
	 /***** Get data of note *****/
	 row = mysql_fetch_row (mysql_res);
	 TL_GetDataOfNoteFromRow (row,SocNot);
	}
      else
	 /***** Reset fields of note *****/
	 TL_ResetNote (SocNot);

      /***** Free structure that stores the query result *****/
      DB_FreeMySQLResult (&mysql_res);
     }
   else
      /***** Reset fields of note *****/
      TL_ResetNote (SocNot);
  }

/*****************************************************************************/
/******************* Get data of comment using its code **********************/
/*****************************************************************************/

static void TL_GetDataOfCommByCod (struct TL_Comment *SocCom)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;

   if (SocCom->PubCod > 0)
     {
      /***** Get data of comment from database *****/
      if (DB_QuerySELECT (&mysql_res,"can not get data of comment",
			  "SELECT social_pubs.PubCod,"				// row[0]
				 "social_pubs.PublisherCod,"			// row[1]
				 "social_pubs.NotCod,"				// row[2]
				 "UNIX_TIMESTAMP(social_pubs.TimePublish),"	// row[3]
				 "social_comments.Content,"			// row[4]
				 "social_comments.MediaName,"			// row[5]
				 "social_comments.MediaType,"			// row[6]
				 "social_comments.MediaTitle,"			// row[7]
				 "social_comments.MediaURL"			// row[8]
			  " FROM social_pubs,social_comments"
			  " WHERE social_pubs.PubCod=%ld"
			  " AND social_pubs.PubType=%u"
			  " AND social_pubs.PubCod=social_comments.PubCod",
			  SocCom->PubCod,(unsigned) TL_PUB_COMMENT_TO_NOTE))
	{
	 /***** Get data of comment *****/
	 row = mysql_fetch_row (mysql_res);
	 TL_GetDataOfCommentFromRow (row,SocCom);
	}
      else
	 /***** Reset fields of comment *****/
	 TL_ResetComment (SocCom);

      /***** Free structure that stores the query result *****/
      DB_FreeMySQLResult (&mysql_res);
     }
   else
      /***** Reset fields of comment *****/
      TL_ResetComment (SocCom);
  }

/*****************************************************************************/
/****************** Get data of publication using its code *******************/
/*****************************************************************************/

static void TL_GetDataOfPublicationFromRow (MYSQL_ROW row,struct TL_Publication *SocPub)
  {
   const TL_TopMessage_t TopMessages[TL_NUM_PUB_TYPES] =
     {
      TL_TOP_MESSAGE_NONE,		// TL_PUB_UNKNOWN
      TL_TOP_MESSAGE_NONE,		// TL_PUB_ORIGINAL_NOTE
      TL_TOP_MESSAGE_SHARED,		// TL_PUB_SHARED_NOTE
      TL_TOP_MESSAGE_COMMENTED,	// TL_PUB_COMMENT_TO_NOTE
     };

   /***** Get code of publication (row[0]) *****/
   SocPub->PubCod       = Str_ConvertStrCodToLongCod (row[0]);

   /***** Get note code (row[1]) *****/
   SocPub->NotCod       = Str_ConvertStrCodToLongCod (row[1]);

   /***** Get publisher's code (row[2]) *****/
   SocPub->PublisherCod = Str_ConvertStrCodToLongCod (row[2]);

   /***** Get type of publication (row[3]) *****/
   SocPub->PubType      = TL_GetPubTypeFromStr ((const char *) row[3]);
   SocPub->TopMessage   = TopMessages[SocPub->PubType];

   /***** Get time of the note (row[4]) *****/
   SocPub->DateTimeUTC  = Dat_GetUNIXTimeFromStr (row[4]);
  }

/*****************************************************************************/
/************************ Get data of note from row **************************/
/*****************************************************************************/

static void TL_GetDataOfNoteFromRow (MYSQL_ROW row,struct TL_Note *SocNot)
  {
   /***** Get code (row[0]) *****/
   SocNot->NotCod      = Str_ConvertStrCodToLongCod (row[0]);

   /***** Get note type (row[1]) *****/
   SocNot->NoteType    = TL_GetNoteTypeFromStr ((const char *) row[1]);

   /***** Get file/post... code (row[2]) *****/
   SocNot->Cod         = Str_ConvertStrCodToLongCod (row[2]);

   /***** Get (from) user code (row[3]) *****/
   SocNot->UsrCod      = Str_ConvertStrCodToLongCod (row[3]);

   /***** Get hierarchy code (row[4]) *****/
   SocNot->HieCod      = Str_ConvertStrCodToLongCod (row[4]);

   /***** File/post... unavailable (row[5]) *****/
   SocNot->Unavailable = (row[5][0] == 'Y');

   /***** Get time of the note (row[6]) *****/
   SocNot->DateTimeUTC = Dat_GetUNIXTimeFromStr (row[6]);

   /***** Get number of times this note has been shared *****/
   TL_UpdateNumTimesANoteHasBeenShared (SocNot);

   /***** Get number of times this note has been favourited *****/
   TL_GetNumTimesANoteHasBeenFav (SocNot);
  }

/*****************************************************************************/
/******** Get publication type from string number coming from database *******/
/*****************************************************************************/

static TL_PubType_t TL_GetPubTypeFromStr (const char *Str)
  {
   unsigned UnsignedNum;

   if (sscanf (Str,"%u",&UnsignedNum) == 1)
      if (UnsignedNum < TL_NUM_PUB_TYPES)
         return (TL_PubType_t) UnsignedNum;

   return TL_PUB_UNKNOWN;
  }

/*****************************************************************************/
/********* Get note type from string number coming from database *************/
/*****************************************************************************/

static TL_NoteType_t TL_GetNoteTypeFromStr (const char *Str)
  {
   unsigned UnsignedNum;

   if (sscanf (Str,"%u",&UnsignedNum) == 1)
      if (UnsignedNum < TL_NUM_NOTE_TYPES)
         return (TL_NoteType_t) UnsignedNum;

   return TL_NOTE_UNKNOWN;
  }

/*****************************************************************************/
/********************** Get data of comment from row *************************/
/*****************************************************************************/

static void TL_GetDataOfCommentFromRow (MYSQL_ROW row,struct TL_Comment *SocCom)
  {
   /*
   row[0]: PubCod
   row[1]: PublisherCod
   row[2]: NotCod
   row[3]: TimePublish
   row[4]: Content
   row[5]: MediaName
   row[6]: MediaType
   row[7]: MediaTitle
   row[8]: MediaURL
    */
   /***** Get code of comment (row[0]) *****/
   SocCom->PubCod      = Str_ConvertStrCodToLongCod (row[0]);

   /***** Get (from) user code (row[1]) *****/
   SocCom->UsrCod      = Str_ConvertStrCodToLongCod (row[1]);

   /***** Get code of note (row[2]) *****/
   SocCom->NotCod      = Str_ConvertStrCodToLongCod (row[2]);

   /***** Get time of the note (row[3]) *****/
   SocCom->DateTimeUTC = Dat_GetUNIXTimeFromStr (row[3]);

   /***** Get content (row[4]) *****/
   Str_Copy (SocCom->Content,row[4],
             Cns_MAX_BYTES_LONG_TEXT);

   /***** Get number of times this comment has been favourited *****/
   TL_GetNumTimesACommHasBeenFav (SocCom);

   /****** Get media data (row[5], row[6], row[7], row[8]) *****/
   Med_GetMediaDataFromRow (row[5],row[6],row[7],row[8],&SocCom->Media);
  }

/*****************************************************************************/
/*************************** Reset fields of note ****************************/
/*****************************************************************************/

static void TL_ResetNote (struct TL_Note *SocNot)
  {
   SocNot->NotCod      = -1L;
   SocNot->NoteType    = TL_NOTE_UNKNOWN;
   SocNot->UsrCod      = -1L;
   SocNot->HieCod      = -1L;
   SocNot->Cod         = -1L;
   SocNot->Unavailable = false;
   SocNot->DateTimeUTC = (time_t) 0;
   SocNot->NumShared   = 0;
  }

/*****************************************************************************/
/************************** Reset fields of comment **************************/
/*****************************************************************************/

static void TL_ResetComment (struct TL_Comment *SocCom)
  {
   SocCom->PubCod      = -1L;
   SocCom->UsrCod      = -1L;
   SocCom->NotCod      = -1L;
   SocCom->DateTimeUTC = (time_t) 0;
   SocCom->Content[0]  = '\0';
  }

/*****************************************************************************/
/******************* Clear unused old timelines in database ******************/
/*****************************************************************************/

void TL_ClearOldTimelinesDB (void)
  {
   /***** Remove timelines for expired sessions *****/
   DB_QueryDELETE ("can not remove old timelines",
		   "DELETE LOW_PRIORITY FROM social_timelines"
                   " WHERE SessionId NOT IN (SELECT SessionId FROM sessions)");
  }

/*****************************************************************************/
/**************** Clear timeline for this session in database ****************/
/*****************************************************************************/

static void TL_ClearTimelineThisSession (void)
  {
   /***** Remove timeline for this session *****/
   DB_QueryDELETE ("can not remove timeline",
		   "DELETE FROM social_timelines"
		   " WHERE SessionId='%s'",
		   Gbl.Session.Id);
  }

/*****************************************************************************/
/****** Add just retrieved notes to current timeline for this session ********/
/*****************************************************************************/

static void TL_AddNotesJustRetrievedToTimelineThisSession (void)
  {
   DB_QueryINSERT ("can not insert notes in timeline",
		   "INSERT IGNORE INTO social_timelines"
	           " (SessionId,NotCod)"
	           " SELECT DISTINCTROW '%s',NotCod FROM not_codes",
		   Gbl.Session.Id);
  }

/*****************************************************************************/
/****************** Get notification of a new publication ********************/
/*****************************************************************************/

void TL_GetNotifPublication (char SummaryStr[Ntf_MAX_BYTES_SUMMARY + 1],
                             char **ContentStr,
                             long PubCod,bool GetContent)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   struct TL_Publication SocPub;
   struct TL_Note SocNot;
   char Content[Cns_MAX_BYTES_LONG_TEXT + 1];
   size_t Length;
   bool ContentCopied = false;

   /***** Return nothing on error *****/
   SocPub.PubType = TL_PUB_UNKNOWN;
   SummaryStr[0] = '\0';	// Return nothing on error
   Content[0] = '\0';

   /***** Get summary and content from post from database *****/
   if (DB_QuerySELECT (&mysql_res,"can not get data of publication",
		       "SELECT PubCod,"				// row[0]
			      "NotCod,"				// row[1]
			      "PublisherCod,"			// row[2]
			      "PubType,"			// row[3]
			      "UNIX_TIMESTAMP(TimePublish)"	// row[4]
		       " FROM social_pubs WHERE PubCod=%ld",
		       PubCod) == 1)   // Result should have a unique row
     {
      /* Get data of publication */
      row = mysql_fetch_row (mysql_res);
      TL_GetDataOfPublicationFromRow (row,&SocPub);
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   /***** Get summary and content *****/
   switch (SocPub.PubType)
     {
      case TL_PUB_UNKNOWN:
	 break;
      case TL_PUB_ORIGINAL_NOTE:
      case TL_PUB_SHARED_NOTE:
	 /* Get data of note */
	 SocNot.NotCod = SocPub.NotCod;
	 TL_GetDataOfNoteByCod (&SocNot);

	 if (SocNot.NoteType == TL_NOTE_POST)
	   {
	    /***** Get content of post from database *****/
	    // TODO: What happens if content is empty and an image is attached?
	    if (DB_QuerySELECT (&mysql_res,"can not get the content of a post",
			        "SELECT Content FROM social_posts"
				" WHERE PstCod=%ld",
				SocNot.Cod) == 1)   // Result should have a unique row
	      {
	       /***** Get row *****/
	       row = mysql_fetch_row (mysql_res);

	       /****** Get content (row[0]) *****/
	       Str_Copy (Content,row[0],
	                 Cns_MAX_BYTES_LONG_TEXT);
	      }

	    /***** Free structure that stores the query result *****/
            DB_FreeMySQLResult (&mysql_res);

	    /***** Copy content string *****/
	    if (GetContent)
	      {
	       Length = strlen (Content);
	       if ((*ContentStr = (char *) malloc (Length + 1)) != NULL)
		 {
		  Str_Copy (*ContentStr,Content,
		            Length);
		  ContentCopied = true;
		 }
	      }

	    /***** Copy summary string *****/
	    Str_LimitLengthHTMLStr (Content,Ntf_MAX_CHARS_SUMMARY);
	    Str_Copy (SummaryStr,Content,
	              Ntf_MAX_BYTES_SUMMARY);
	   }
	 else
	    TL_GetNoteSummary (&SocNot,SummaryStr);
	 break;
      case TL_PUB_COMMENT_TO_NOTE:
	 /***** Get content of post from database *****/
	 // TODO: What happens if content is empty and an image is attached?
	 if (DB_QuerySELECT (&mysql_res,"can not get the content"
				        " of a comment to a note",
			     "SELECT Content FROM social_comments"
			     " WHERE PubCod=%ld",
			     SocPub.PubCod) == 1)   // Result should have a unique row
	   {
	    /***** Get row *****/
	    row = mysql_fetch_row (mysql_res);

	    /****** Get content (row[0]) *****/
	    Str_Copy (Content,row[0],
	              Cns_MAX_BYTES_LONG_TEXT);
	   }

	 /***** Free structure that stores the query result *****/
	 DB_FreeMySQLResult (&mysql_res);

	 /***** Copy content string *****/
	 if (GetContent)
	   {
	    Length = strlen (Content);
	    if ((*ContentStr = (char *) malloc (Length + 1)) != NULL)
	      {
	       Str_Copy (*ContentStr,Content,
	                 Length);
	       ContentCopied = true;
	      }
	   }

	 /***** Copy summary string *****/
	 Str_LimitLengthHTMLStr (Content,Ntf_MAX_CHARS_SUMMARY);
	 Str_Copy (SummaryStr,Content,
	           Ntf_MAX_BYTES_SUMMARY);
	 break;
     }

   /***** Create empty content string if nothing copied *****/
   if (GetContent && !ContentCopied)
      if ((*ContentStr = (char *) malloc (1)) != NULL)
         (*ContentStr)[0] = '\0';
  }

/*****************************************************************************/
/*** Create a notification about mention for any nickname in a publication ***/
/*****************************************************************************/
/*
 Example: "The user @rms says..."
                     ^ ^
         PtrStart ___| |___ PtrEnd
                 Length = 3
*/
static void Str_AnalyzeTxtAndStoreNotifyEventToMentionedUsrs (long PubCod,const char *Txt)
  {
   const char *Ptr;
   bool IsNickname;
   struct
     {
      const char *PtrStart;
      const char *PtrEnd;
      size_t Length;		// Length of the nickname
     } Nickname;
   struct UsrData UsrDat;
   bool ItsMe;
   bool CreateNotif;
   bool NotifyByEmail;

   /***** Initialize structure with user's data *****/
   Usr_UsrDataConstructor (&UsrDat);

   /***** Find nicknames and create notifications *****/
   for (Ptr = Txt;
	*Ptr;)
      /* Check if the next char is the start of a nickname */
      if ((int) *Ptr == (int) '@')
	{
	 /* Find nickname end */
	 Ptr++;	// Points to first character after @
         Nickname.PtrStart = Ptr;

	 /* A nick can have digits, letters and '_'  */
	 for (;
	      *Ptr;
	      Ptr++)
	    if (!((*Ptr >= 'a' && *Ptr <= 'z') ||
		  (*Ptr >= 'A' && *Ptr <= 'Z') ||
		  (*Ptr >= '0' && *Ptr <= '9') ||
		  (*Ptr == '_')))
	       break;

	 /* Calculate length of this nickname */
	 Nickname.PtrEnd = Ptr - 1;
         Nickname.Length = (size_t) (Ptr - Nickname.PtrStart);

	 /* A nick (without arroba) must have a number of characters
            Nck_MIN_BYTES_NICKNAME_WITHOUT_ARROBA <= Length <= Nck_MAX_BYTES_NICKNAME_WITHOUT_ARROBA */
	 IsNickname = (Nickname.Length >= Nck_MIN_BYTES_NICKNAME_WITHOUT_ARROBA &&
	               Nickname.Length <= Nck_MAX_BYTES_NICKNAME_WITHOUT_ARROBA);

	 if (IsNickname)
	   {
	    /* Copy nickname */
	    strncpy (UsrDat.Nickname,Nickname.PtrStart,Nickname.Length);
	    UsrDat.Nickname[Nickname.Length] = '\0';

	    if ((UsrDat.UsrCod = Nck_GetUsrCodFromNickname (UsrDat.Nickname)) > 0)
	      {
	       ItsMe = Usr_ItsMe (UsrDat.UsrCod);
	       if (!ItsMe)	// Not me
		 {
		  /* Get user's data */
		  Usr_GetAllUsrDataFromUsrCod (&UsrDat);

		  /* Create notification for the mentioned user *****/
		  CreateNotif = (UsrDat.Prefs.NotifNtfEvents & (1 << Ntf_EVENT_TIMELINE_MENTION));
		  if (CreateNotif)
		    {
		     NotifyByEmail = (UsrDat.Prefs.EmailNtfEvents & (1 << Ntf_EVENT_TIMELINE_MENTION));
		     Ntf_StoreNotifyEventToOneUser (Ntf_EVENT_TIMELINE_MENTION,&UsrDat,PubCod,
						    (Ntf_Status_t) (NotifyByEmail ? Ntf_STATUS_BIT_EMAIL :
										    0));
		    }
		 }
	      }
	   }
	}
      /* The next char is not the start of a nickname */
      else	// Character != '@'
         Ptr++;

   /***** Free memory used for user's data *****/
   Usr_UsrDataDestructor (&UsrDat);
  }

/*****************************************************************************/
/****************** Get number of publications from a user *******************/
/*****************************************************************************/

unsigned long TL_GetNumPubsUsr (long UsrCod)
  {
   /***** Get number of posts from a user from database *****/
   return DB_QueryCOUNT ("can not get number of publications from a user",
			 "SELECT COUNT(*) FROM social_pubs"
			 " WHERE PublisherCod=%ld",
			 UsrCod);
  }