// swad_media.c: processing of image/video uploaded in a form

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

#define _GNU_SOURCE         	// For strcasestr, asprintf
#include <linux/limits.h>	// For PATH_MAX
#include <stdbool.h>		// For boolean type
#include <stdio.h>		// For asprintf
#include <stdlib.h>		// For exit, system, malloc, free, etc
#include <string.h>		// For string functions
#include <sys/stat.h>		// For lstat
#include <sys/types.h>		// For lstat
#include <sys/wait.h>		// For the macro WEXITSTATUS
#include <unistd.h>		// For unlink, lstat

#include "swad_config.h"
#include "swad_global.h"
#include "swad_file.h"
#include "swad_file_browser.h"
#include "swad_form.h"
#include "swad_media.h"

/*****************************************************************************/
/****************************** Public constants *****************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Internal constants ****************************/
/*****************************************************************************/

const char *Med_StringsTypeDB[Med_NUM_TYPES] =
  {
   "none",	// Med_NONE
   "jpg",	// Med_JPG
   "gif",	// Med_GIF
   "mp4",	// Med_MP4
   "webm",	// Med_WEBM
   "ogg",	// Med_OGG
   "youtube",	// Med_YOUTUBE
   };

const char *Med_Extensions[Med_NUM_TYPES] =
  {
   "",		// Med_NONE
   "jpg",	// Med_JPG
   "gif",	// Med_GIF
   "mp4",	// Med_MP4
   "webm",	// Med_WEBM
   "ogg",	// Med_OGG
   "",		// Med_YOUTUBE
   };

#define Med_MAX_SIZE_GIF (5UL * 1024UL * 1024UL)	// 5 MiB
#define Med_MAX_SIZE_MP4 (5UL * 1024UL * 1024UL)	// 5 MiB

/*****************************************************************************/
/****************************** Internal types *******************************/
/*****************************************************************************/

#define Med_NUM_FORM_TYPES 3

typedef enum
  {
   Med_FORM_UNKNOWN = 0,
   Med_FORM_FILE    = 1,
   Med_FORM_EMBED   = 2,
  } Med_FormType_t;

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/************************* Internal global variables *************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Internal prototypes ***************************/
/*****************************************************************************/

static Med_Action_t Med_GetMediaActionFromForm (const char *ParamAction);
static Med_FormType_t Usr_GetFormTypeFromForm (void);
static void Usr_GetURLFromForm (const char *ParamName,struct Media *Media);
static void Usr_GetTitleFromForm (const char *ParamName,struct Media *Media);
static void Med_GetAndProcessFileFromForm (const char *ParamFile,
                                           struct Media *Media);
static bool Med_DetectIfAnimated (struct Media *Media,
			          const char PathMedPrivTmp[PATH_MAX + 1],
			          const char PathFileOrg[PATH_MAX + 1]);

static void Med_ProcessJPG (struct Media *Media,
			    const char PathMedPrivTmp[PATH_MAX + 1],
			    const char PathFileOrg[PATH_MAX + 1]);
static void Med_ProcessGIF (struct Media *Media,
			    const char PathMedPrivTmp[PATH_MAX + 1],
			    const char PathFileOrg[PATH_MAX + 1]);
static void Med_ProcessVideo (struct Media *Media,
			      const char PathMedPrivTmp[PATH_MAX + 1],
			      const char PathFileOrg[PATH_MAX + 1]);

static int Med_ResizeImage (struct Media *Media,
                            const char PathFileOriginal[PATH_MAX + 1],
                            const char PathFileProcessed[PATH_MAX + 1]);
static int Med_GetFirstFrame (const char PathFileOriginal[PATH_MAX + 1],
                              const char PathFileProcessed[PATH_MAX + 1]);

static void Med_GetAndProcessEmbedFromForm (const char *ParamURL,
                                            struct Media *Media);

static bool Med_MoveTmpFileToDefDir (struct Media *Media,
				     const char PathMedPrivTmp[PATH_MAX + 1],
				     const char PathMedPriv[PATH_MAX + 1],
				     const char *Extension);

static void Med_ShowJPG (struct Media *Media,
			 const char PathMedPriv[PATH_MAX + 1],
			 const char *ClassMedia);
static void Med_ShowGIF (struct Media *Media,
			 const char PathMedPriv[PATH_MAX + 1],
			 const char *ClassMedia);
static void Med_ShowVideo (struct Media *Media,
			   const char PathMedPriv[PATH_MAX + 1],
			   const char *ClassMedia);
static void Med_ShowYoutube (struct Media *Media,
			     const char *ClassMedia);

static Med_Type_t Med_GetTypeFromExtAndMIME (const char *Extension,
                                             const char *MIMEType);

/*****************************************************************************/
/********************* Reset media (image/video) fields **********************/
/*****************************************************************************/
// Every struct Media must be initialized with this constructor function after it is declared
// Every call to constructor must have a call to destructor

void Med_MediaConstructor (struct Media *Media)
  {
   Med_ResetMediaExceptTitleAndURL (Media);
   Media->Title = NULL;
   Media->URL   = NULL;
  }

/*****************************************************************************/
/***************** Reset image fields except title and URL *******************/
/*****************************************************************************/

void Med_ResetMediaExceptTitleAndURL (struct Media *Media)
  {
   Media->Action = Med_ACTION_NO_MEDIA;
   Media->Status = Med_STATUS_NONE;
   Media->Name[0] = '\0';
   Media->Type = Med_TYPE_NONE;
  }

/*****************************************************************************/
/************************ Free media (image/video) ***************************/
/*****************************************************************************/
// Every call to constructor must have a call to destructor

void Med_MediaDestructor (struct Media *Media)
  {
   Med_FreeMediaTitle (Media);
   Med_FreeMediaURL (Media);
  }

/*****************************************************************************/
/****************************** Free image title *****************************/
/*****************************************************************************/

void Med_FreeMediaTitle (struct Media *Media)
  {
   // Media->Title must be initialized to NULL after declaration
   if (Media->Title)
     {
      free ((void *) Media->Title);
      Media->Title = NULL;
     }
  }

/*****************************************************************************/
/******************************* Free image URL ******************************/
/*****************************************************************************/

void Med_FreeMediaURL (struct Media *Media)
  {
   // Media->URL must be initialized to NULL after declaration
   if (Media->URL)
     {
      free ((void *) Media->URL);
      Media->URL = NULL;
     }
  }

/*****************************************************************************/
/**** Get media name, title and URL from a query result and copy to struct ***/
/*****************************************************************************/

void Med_GetMediaDataFromRow (const char *Name,
			      const char *TypeStr,
                              const char *Title,
                              const char *URL,
                              struct Media *Media)
  {
   size_t Length;

   /***** Copy image name to struct *****/
   Str_Copy (Media->Name,Name,
             Med_BYTES_NAME);

   /***** Convert type string to type *****/
   Media->Type = Med_GetTypeFromStrInDB (TypeStr);

   /***** Set status of media file *****/
   Media->Status = (Media->Type != Med_TYPE_NONE) ? Med_STORED_IN_DB :
	                                            Med_STATUS_NONE;

   /***** Copy image URL to struct *****/
   // Media->URL can be empty or filled with previous value
   // If filled  ==> free it
   Med_FreeMediaURL (Media);
   if (URL[0])
     {
      /* Get and limit length of the URL */
      Length = strlen (URL);
      if (Length > Cns_MAX_BYTES_WWW)
	  Length = Cns_MAX_BYTES_WWW;

      if ((Media->URL = (char *) malloc (Length + 1)) == NULL)
	 Lay_ShowErrorAndExit ("Error allocating memory for image URL.");
      Str_Copy (Media->URL,URL,
                Length);
     }

   /***** Copy image title to struct *****/
   // Media->Title can be empty or filled with previous value
   // If filled  ==> free it
   Med_FreeMediaTitle (Media);
   if (Title[0])
     {
      /* Get and limit length of the title */
      Length = strlen (Title);
      if (Length > Med_MAX_BYTES_TITLE)
	  Length = Med_MAX_BYTES_TITLE;

      if ((Media->Title = (char *) malloc (Length + 1)) == NULL)
	 Lay_ShowErrorAndExit ("Error allocating memory for image title.");
      Str_Copy (Media->Title,Title,
                Length);
     }
  }

/*****************************************************************************/
/********* Draw input fields to upload an image/video inside a form **********/
/*****************************************************************************/

void Med_PutMediaUploader (int NumMediaInForm,const char *ClassMediaTitURL)
  {
   extern const char *Txt_Image_video;
   extern const char *Txt_Title_attribution;
   extern const char *Txt_Link;
   struct ParamUploadMedia ParamUploadMedia;
   char Id[Frm_MAX_BYTES_ID + 1];

   /***** Set names of parameters depending on number of media in form *****/
   Med_SetParamNames (&ParamUploadMedia,NumMediaInForm);

   /***** Create unique id for this media uploader *****/
   Frm_SetUniqueId (Id);

   /***** Start container *****/
   fprintf (Gbl.F.Out,"<div class=\"MED_UPL_CON\">");	// container

   /***** Action to perform on media *****/
   Par_PutHiddenParamUnsigned (ParamUploadMedia.Action,(unsigned) Med_ACTION_NEW_MEDIA);

   /***** Upload icon *****/
   fprintf (Gbl.F.Out,"<div id=\"%s_ico_upl\""			// <id>_ico_upl
		      " class=\"MED_UPL_ICO_CON\">"
		      "<img src=\"%s/file-image.svg\""
	              " alt=\"%s\" title=\"%s\""
	              " class=\"MED_UPL_ICO ICO_HIGHLIGHT\""
	              " onclick=\"mediaActivateUpload('%s');\" />"
	              "</div>",					// <id>_ico_upl
            Id,
	    Gbl.Prefs.URLIcons,
            Txt_Image_video,Txt_Image_video,
            Id);

   /***** Form type *****/
   fprintf (Gbl.F.Out,"<input type=\"hidden\""
		      " id=\"%s_par_upl\""			// <id>_par_upl
		      " name=\"FormType\" value=\"%u\""
		      " disabled=\"disabled\" />",
	    Id,
            (unsigned) Med_FORM_FILE);

   /***** Embed icon *****/
   fprintf (Gbl.F.Out,"<div id=\"%s_ico_emb\""			// <id>_ico_emb
		      " class=\"MED_UPL_ICO_CON\">"
		      "<img src=\"%s/youtube-brands.svg\""
	              " alt=\"%s\" title=\"%s\""
	              " class=\"MED_UPL_ICO ICO_HIGHLIGHT\""
	              " onclick=\"mediaActivateEmbed('%s');\" />"
	              "</div>",					// <id>_ico_emb
            Id,
	    Gbl.Prefs.URLIcons,
            "YouTube","YouTube",
            Id);

   /***** Form type *****/
   fprintf (Gbl.F.Out,"<input type=\"hidden\""
		      " id=\"%s_par_emb\""			// <id>_par_emb
		      " name=\"FormType\" value=\"%u\""
		      " disabled=\"disabled\" />",
	    Id,
            (unsigned) Med_FORM_EMBED);

   /***** Media file *****/
   fprintf (Gbl.F.Out,"<input id=\"%s_fil\" type=\"file\""	// <id>_fil
	              " name=\"%s\" accept=\"image/,video/\""
	              " disabled=\"disabled\""
	              " style=\"display:none;\" />",		// <id>_fil
	    Id,
            ParamUploadMedia.File);

   /***** Media URL *****/
   fprintf (Gbl.F.Out,"<input id=\"%s_url\" type=\"url\""	// <id>_url
		      " name=\"%s\" placeholder=\"%s\""
                      " class=\"%s\" maxlength=\"%u\" value=\"\""
	              " disabled=\"disabled\""
	              " style=\"display:none;\" />",		// <id>_url
	    Id,
            ParamUploadMedia.URL,Txt_Link,
            ClassMediaTitURL,Cns_MAX_CHARS_WWW);

   /***** Media title *****/
   fprintf (Gbl.F.Out,"<input id=\"%s_tit\" type=\"text\""	// <id>_tit
		      " name=\"%s\" placeholder=\"%s\""
                      " class=\"%s\" maxlength=\"%u\" value=\"\""
	              " disabled=\"disabled\""
	              " style=\"display:none;\" />",		// <id>_tit
	    Id,
            ParamUploadMedia.Title,Txt_Title_attribution,
            ClassMediaTitURL,Med_MAX_CHARS_TITLE);

   /***** End container *****/
   fprintf (Gbl.F.Out,"</div>");			// container
  }

/*****************************************************************************/
/******************** Get media (image/video) from form **********************/
/*****************************************************************************/
// If NumMediaInForm < 0, params have no suffix
// If NumMediaInForm >= 0, the number is a suffix of the params

void Med_GetMediaFromForm (int NumMediaInForm,struct Media *Media,
                           void (*GetMediaFromDB) (int NumMediaInForm,struct Media *Media))
  {
   struct ParamUploadMedia ParamUploadMedia;
   Med_FormType_t FormType;

   /***** Set names of parameters depending on number of media in form *****/
   Med_SetParamNames (&ParamUploadMedia,NumMediaInForm);

   /***** Get action and initialize media (image/video)
          (except title, that will be get after the media file) *****/
   Media->Action = Med_GetMediaActionFromForm (ParamUploadMedia.Action);
   Media->Status = Med_STATUS_NONE;
   Media->Name[0] = '\0';
   Media->Type = Med_TYPE_NONE;

   /***** Get form type *****/
   FormType = Usr_GetFormTypeFromForm ();

   /***** Get the media (image/video) name and the file *****/
   switch (Media->Action)
     {
      case Med_ACTION_NEW_MEDIA:	// Upload new image/video
         /***** Get new media *****/
	 switch (FormType)
	   {
	    case Med_FORM_FILE:
	       /***** Get image/video (if present ==> process and create temporary file) *****/
	       Med_GetAndProcessFileFromForm (ParamUploadMedia.File,Media);
	       switch (Media->Status)
		 {
		  case Med_STATUS_NONE:	// No new image/video received
		     Media->Action = Med_ACTION_NO_MEDIA;
		     Media->Name[0] = '\0';
		     Media->Type = Med_TYPE_NONE;
		     break;
		  case Med_RECEIVED:	// New image/video received, but not processed
		     Media->Status = Med_STATUS_NONE;
		     Media->Name[0] = '\0';
		     Media->Type = Med_TYPE_NONE;
		     break;
		  case Med_PROCESSED:
	             Usr_GetURLFromForm (ParamUploadMedia.URL,Media);
	             Usr_GetTitleFromForm (ParamUploadMedia.Title,Media);
	             break;
		  default:
		     break;
		 }
	       break;
	    case Med_FORM_EMBED:
	       /***** Get and process embed URL from form *****/
	       Med_GetAndProcessEmbedFromForm (ParamUploadMedia.URL,Media);
	       break;
	    default:
	       break;
	   }
	 break;
      case Med_ACTION_KEEP_MEDIA:	// Keep current image/video unchanged
	 /***** Get media name *****/
	 if (GetMediaFromDB != NULL)
	    GetMediaFromDB (NumMediaInForm,Media);
	 break;
      case Med_ACTION_CHANGE_MEDIA:	// Replace old image/video by new one
	 switch (FormType)
	   {
	    case Med_FORM_FILE:
	       /***** Get image/video (if present ==> process and create temporary file) *****/
	       Med_GetAndProcessFileFromForm (ParamUploadMedia.File,Media);
	       switch (Media->Status)
		 {
		  case Med_PROCESSED:
	             Usr_GetURLFromForm (ParamUploadMedia.URL,Media);
	             Usr_GetTitleFromForm (ParamUploadMedia.Title,Media);
	             break;
		  default:
		     break;
		 }
	       break;
	    case Med_FORM_EMBED:
	       /***** Get and process embed URL from form *****/
	       Med_GetAndProcessEmbedFromForm (ParamUploadMedia.URL,Media);
	       break;
	    default:
	       break;
	   }
	 if (Media->Status != Med_PROCESSED &&	// No new media received-processed successfully
	     GetMediaFromDB != NULL)
	    /* Get media (image/video) name */
	    GetMediaFromDB (NumMediaInForm,Media);
	 break;
      case Med_ACTION_NO_MEDIA:		// Do not use image/video (remove current image/video if exists)
         break;
     }
  }

/*****************************************************************************/
/********* Set parameters names depending on number of media in form *********/
/*****************************************************************************/
// If NumMediaInForm <  0, params have no suffix
// If NumMediaInForm >= 0, the number is a suffix of the params

void Med_SetParamNames (struct ParamUploadMedia *ParamUploadMedia,int NumMediaInForm)
  {
   if (NumMediaInForm < 0)	// One unique media in form ==> no suffix needed
     {
      Str_Copy (ParamUploadMedia->Action,"MedAct",
                Med_MAX_BYTES_PARAM_UPLOAD_MEDIA);
      Str_Copy (ParamUploadMedia->File  ,"MedFil",
                Med_MAX_BYTES_PARAM_UPLOAD_MEDIA);
      Str_Copy (ParamUploadMedia->Title ,"MedTit",
                Med_MAX_BYTES_PARAM_UPLOAD_MEDIA);
      Str_Copy (ParamUploadMedia->URL   ,"MedURL",
                Med_MAX_BYTES_PARAM_UPLOAD_MEDIA);
     }
   else				// Several video/images in form ==> add suffix
     {
      snprintf (ParamUploadMedia->Action,sizeof (ParamUploadMedia->Action),
	        "MedAct%u",
		NumMediaInForm);
      snprintf (ParamUploadMedia->File  ,sizeof (ParamUploadMedia->File),
	        "MedFil%u",
		NumMediaInForm);
      snprintf (ParamUploadMedia->Title ,sizeof (ParamUploadMedia->Title),
	        "MedTit%u",
		NumMediaInForm);
      snprintf (ParamUploadMedia->URL   ,sizeof (ParamUploadMedia->URL),
	        "MedURL%u",
		NumMediaInForm);
     }
  }

/*****************************************************************************/
/************************* Get media action from form ************************/
/*****************************************************************************/

static Med_Action_t Med_GetMediaActionFromForm (const char *ParamAction)
  {
   /***** Get parameter with the action to perform on media *****/
   return (Med_Action_t)
	  Par_GetParToUnsignedLong (ParamAction,
                                    0,
                                    Med_NUM_ACTIONS - 1,
                                    (unsigned long) Med_ACTION_DEFAULT);
  }

/*****************************************************************************/
/********************* Get from form the type of form ************************/
/*****************************************************************************/

static Med_FormType_t Usr_GetFormTypeFromForm (void)
  {
   return (Med_FormType_t) Par_GetParToUnsignedLong ("FormType",
                                                     0,
                                                     Med_NUM_FORM_TYPES - 1,
                                                     (unsigned long) Med_FORM_UNKNOWN);
  }

/*****************************************************************************/
/********************* Get from form the type of form ************************/
/*****************************************************************************/

static void Usr_GetURLFromForm (const char *ParamName,struct Media *Media)
  {
   char URL[Cns_MAX_BYTES_WWW + 1];
   size_t Length;

   /***** Get media URL from form *****/
   Par_GetParToText (ParamName,URL,Cns_MAX_BYTES_WWW);
   /* If the URL coming from the form is empty, keep current media URL unchanged
      If not empty, copy it to current media URL */
   if ((Length = strlen (URL)) > 0)
     {
      /* Overwrite current URL (empty or coming from database)
         with the URL coming from the form */
      Med_FreeMediaURL (Media);
      if ((Media->URL = (char *) malloc (Length + 1)) == NULL)
	 Lay_ShowErrorAndExit ("Error allocating memory for media URL.");
      Str_Copy (Media->URL,URL,
                Length);
     }
  }

/*****************************************************************************/
/********************* Get from form the type of form ************************/
/*****************************************************************************/

static void Usr_GetTitleFromForm (const char *ParamName,struct Media *Media)
  {
   char Title[Med_MAX_BYTES_TITLE + 1];
   size_t Length;

   /***** Get image/video title from form *****/
   Par_GetParToText (ParamName,Title,Med_MAX_BYTES_TITLE);
   /* If the title coming from the form is empty, keep current media title unchanged
      If not empty, copy it to current media title */
   if ((Length = strlen (Title)) > 0)
     {
      /* Overwrite current title (empty or coming from database)
         with the title coming from the form */
      Med_FreeMediaTitle (Media);
      if ((Media->Title = (char *) malloc (Length + 1)) == NULL)
	 Lay_ShowErrorAndExit ("Error allocating memory for media title.");
      Str_Copy (Media->Title,Title,
                Length);
     }
  }

/*****************************************************************************/
/**************************** Get media from form ****************************/
/*****************************************************************************/

static void Med_GetAndProcessFileFromForm (const char *ParamFile,
                                           struct Media *Media)
  {
   struct Param *Param;
   char FileNameImgSrc[PATH_MAX + 1];
   char *PtrExtension;
   size_t LengthExtension;
   char MIMEType[Brw_MAX_BYTES_MIME_TYPE + 1];
   char PathMedPriv[PATH_MAX + 1];
   char PathMedPrivTmp[PATH_MAX + 1];
   char PathFileOrg[PATH_MAX + 1];	// Full name of original uploaded file

   /***** Set media status *****/
   Media->Status = Med_STATUS_NONE;

   /***** Get filename and MIME type *****/
   Param = Fil_StartReceptionOfFile (ParamFile,FileNameImgSrc,MIMEType);
   if (!FileNameImgSrc[0])	// No file present
      return;

   /* Get filename extension */
   if ((PtrExtension = strrchr (FileNameImgSrc,(int) '.')) == NULL)
      return;
   // PtrExtension now points to last '.' in file

   PtrExtension++;
   // PtrExtension now points to first char in extension

   LengthExtension = strlen (PtrExtension);
   if (LengthExtension < Fil_MIN_BYTES_FILE_EXTENSION ||
       LengthExtension > Fil_MAX_BYTES_FILE_EXTENSION)
      return;

   /* Check extension */
   Media->Type = Med_GetTypeFromExtAndMIME (PtrExtension,MIMEType);
   if (Media->Type == Med_TYPE_NONE)
      return;

   /***** Assign a unique name for the media *****/
   Cry_CreateUniqueNameEncrypted (Media->Name);

   /***** Create private directories if not exist *****/
   /* Create private directory for images/videos if it does not exist */
   snprintf (PathMedPriv,sizeof (PathMedPriv),
	     "%s/%s",
	     Cfg_PATH_SWAD_PRIVATE,Cfg_FOLDER_MEDIA);
   Fil_CreateDirIfNotExists (PathMedPriv);

   /* Create temporary private directory for images/videos if it does not exist */
   snprintf (PathMedPrivTmp,sizeof (PathMedPrivTmp),
	     "%s/%s",
	     PathMedPriv,Cfg_FOLDER_IMG_TMP);
   Fil_CreateDirIfNotExists (PathMedPrivTmp);

   /***** Remove old temporary private files *****/
   Fil_RemoveOldTmpFiles (PathMedPrivTmp,Cfg_TIME_TO_DELETE_IMAGES_TMP_FILES,false);

   /***** End the reception of original not processed media
          (it may be very big) into a temporary file *****/
   Media->Status = Med_STATUS_NONE;
   snprintf (PathFileOrg,sizeof (PathFileOrg),
	     "%s/%s_original.%s",
	     PathMedPrivTmp,Media->Name,PtrExtension);

   if (Fil_EndReceptionOfFile (PathFileOrg,Param))	// Success
     {
      Media->Status = Med_RECEIVED;

      /***** Detect if animated GIF *****/
      if (Media->Type == Med_GIF)
	 if (!Med_DetectIfAnimated (Media,PathMedPrivTmp,PathFileOrg))
            Media->Type = Med_JPG;

      /***** Process media depending on the media file extension *****/
      switch (Media->Type)
        {
         case Med_JPG:
            Med_ProcessJPG (Media,PathMedPrivTmp,PathFileOrg);
            break;
         case Med_GIF:
            Med_ProcessGIF (Media,PathMedPrivTmp,PathFileOrg);
            break;
         case Med_MP4:
         case Med_WEBM:
         case Med_OGG:
            Med_ProcessVideo (Media,PathMedPrivTmp,PathFileOrg);
            break;
         default:
            break;
        }
     }

   /***** Remove temporary original file *****/
   if (Fil_CheckIfPathExists (PathFileOrg))
      unlink (PathFileOrg);
  }

/*****************************************************************************/
/********************* Detect if a GIF image is animated *********************/
/*****************************************************************************/
// Return true if animated
// Return false if static or error

static bool Med_DetectIfAnimated (struct Media *Media,
			          const char PathMedPrivTmp[PATH_MAX + 1],
			          const char PathFileOrg[PATH_MAX + 1])
  {
   char PathFileTxtTmp[PATH_MAX + 1];
   char Command[128 + PATH_MAX * 2];
   int ReturnCode;
   FILE *FileTxtTmp;	// Temporary file with the output of the command
   int NumFrames = 0;

   /***** Build path to temporary text file *****/
   snprintf (PathFileTxtTmp,sizeof (PathFileTxtTmp),
	     "%s/%s.txt",
             PathMedPrivTmp,Media->Name);

   /***** Execute system command to get number of frames in GIF *****/
   snprintf (Command,sizeof (Command),
	     "identify -format '%%n\n' %s | head -1 > %s",
             PathFileOrg,PathFileTxtTmp);
   ReturnCode = system (Command);
   if (ReturnCode == -1)
      return false;		// Error
   ReturnCode = WEXITSTATUS(ReturnCode);
   if (ReturnCode != 0)
      return false;		// Error

   /***** Read temporary file *****/
   if ((FileTxtTmp = fopen (PathFileTxtTmp,"rb")) == NULL)
      return false;		// Error
   if (fscanf (FileTxtTmp,"%d",&NumFrames) != 1)
      return false;		// Error
   fclose (FileTxtTmp);

   /***** Remove temporary file *****/
   unlink (PathFileTxtTmp);

   return (NumFrames > 1);	// NumFrames > 1 ==> Animated
  }

/*****************************************************************************/
/************* Process original image generating processed JPG ***************/
/*****************************************************************************/

static void Med_ProcessJPG (struct Media *Media,
			    const char PathMedPrivTmp[PATH_MAX + 1],
			    const char PathFileOrg[PATH_MAX + 1])
  {
   extern const char *Txt_The_file_could_not_be_processed_successfully;
   char PathFileJPGTmp[PATH_MAX + 1];	// Full name of temporary processed file

   /***** Convert original media to temporary JPG processed file
	  by calling to program that makes the conversion *****/
   snprintf (PathFileJPGTmp,sizeof (PathFileJPGTmp),
	     "%s/%s.%s",
	     PathMedPrivTmp,Media->Name,Med_Extensions[Med_JPG]);
   if (Med_ResizeImage (Media,PathFileOrg,PathFileJPGTmp) == 0)	// On success ==> 0 is returned
      /* Success */
      Media->Status = Med_PROCESSED;
   else // Error processing media
     {
      /* Remove temporary destination media file */
      if (Fil_CheckIfPathExists (PathFileJPGTmp))
	 unlink (PathFileJPGTmp);

      /* Show error alert */
      Ale_ShowAlert (Ale_ERROR,Txt_The_file_could_not_be_processed_successfully);
     }
  }

/*****************************************************************************/
/******* Process original GIF image generating processed PNG and GIF *********/
/*****************************************************************************/

static void Med_ProcessGIF (struct Media *Media,
			    const char PathMedPrivTmp[PATH_MAX + 1],
			    const char PathFileOrg[PATH_MAX + 1])
  {
   extern const char *Txt_The_file_could_not_be_processed_successfully;
   extern const char *Txt_The_size_of_the_file_exceeds_the_maximum_allowed_X;
   struct stat FileStatus;
   char PathFilePNGTmp[PATH_MAX + 1];	// Full name of temporary processed file
   char PathFileGIFTmp[PATH_MAX + 1];	// Full name of temporary processed file
   char FileSizeStr[Fil_MAX_BYTES_FILE_SIZE_STRING + 1];

   /***** Check size of media file *****/
   if (lstat (PathFileOrg,&FileStatus) == 0)	// On success ==> 0 is returned
     {
      /* Success */
      if (FileStatus.st_size <= (__off_t) Med_MAX_SIZE_GIF)
	{
	 /* File size correct */
	 /***** Get first frame of orifinal GIF file
		and save it on temporary PNG file */
	 snprintf (PathFilePNGTmp,sizeof (PathFilePNGTmp),
		   "%s/%s.png",
		   PathMedPrivTmp,Media->Name);
	 if (Med_GetFirstFrame (PathFileOrg,PathFilePNGTmp) == 0)	// On success ==> 0 is returned
	   {
	    /* Success */
	    /***** Move original GIF file to temporary GIF file *****/
	    snprintf (PathFileGIFTmp,sizeof (PathFileGIFTmp),
		      "%s/%s.%s",
		      PathMedPrivTmp,Media->Name,Med_Extensions[Med_GIF]);
	    if (rename (PathFileOrg,PathFileGIFTmp))	// Fail
	      {
	       /* Remove temporary PNG file */
	       if (Fil_CheckIfPathExists (PathFilePNGTmp))
		  unlink (PathFilePNGTmp);

	       /* Show error alert */
	       Ale_ShowAlert (Ale_ERROR,Txt_The_file_could_not_be_processed_successfully);
	      }
	    else					// Success
	       Media->Status = Med_PROCESSED;
	   }
	 else // Error getting first frame
	   {
	    /* Remove temporary PNG file */
	    if (Fil_CheckIfPathExists (PathFilePNGTmp))
	       unlink (PathFilePNGTmp);

	    /* Show error alert */
	    Ale_ShowAlert (Ale_ERROR,Txt_The_file_could_not_be_processed_successfully);
	   }
	}
      else	// Size exceeded
	{
	 /* Show warning alert */
	 Fil_WriteFileSizeBrief ((double) Med_MAX_SIZE_GIF,FileSizeStr);
	 Ale_ShowAlert (Ale_WARNING,Txt_The_size_of_the_file_exceeds_the_maximum_allowed_X,
			FileSizeStr);
	}
     }
   else // Error getting file data
      /* Show error alert */
      Ale_ShowAlert (Ale_ERROR,Txt_The_file_could_not_be_processed_successfully);
  }

/*****************************************************************************/
/*********** Process original MP4 video generating processed MP4 *************/
/*****************************************************************************/

static void Med_ProcessVideo (struct Media *Media,
			      const char PathMedPrivTmp[PATH_MAX + 1],
			      const char PathFileOrg[PATH_MAX + 1])
  {
   extern const char *Txt_The_file_could_not_be_processed_successfully;
   extern const char *Txt_The_size_of_the_file_exceeds_the_maximum_allowed_X;
   struct stat FileStatus;
   char PathFileTmp[PATH_MAX + 1];	// Full name of temporary processed file
   char FileSizeStr[Fil_MAX_BYTES_FILE_SIZE_STRING + 1];

   /***** Check size of media file *****/
   if (lstat (PathFileOrg,&FileStatus) == 0)	// On success ==> 0 is returned
     {
      /* Success */
      if (FileStatus.st_size <= (__off_t) Med_MAX_SIZE_MP4)
	{
	 /* File size correct */
	 /***** Move original video file to temporary MP4 file *****/
	 snprintf (PathFileTmp,sizeof (PathFileTmp),
		   "%s/%s.%s",
		   PathMedPrivTmp,Media->Name,Med_Extensions[Media->Type]);
	 if (rename (PathFileOrg,PathFileTmp))	// Fail
	    /* Show error alert */
	    Ale_ShowAlert (Ale_ERROR,Txt_The_file_could_not_be_processed_successfully);
	 else						// Success
	    Media->Status = Med_PROCESSED;
	}
      else	// Size exceeded
	{
	 /* Show warning alert */
	 Fil_WriteFileSizeBrief ((double) Med_MAX_SIZE_MP4,FileSizeStr);
	 Ale_ShowAlert (Ale_WARNING,Txt_The_size_of_the_file_exceeds_the_maximum_allowed_X,
			FileSizeStr);
	}
     }
   else // Error getting file data
      /* Show error alert */
      Ale_ShowAlert (Ale_ERROR,Txt_The_file_could_not_be_processed_successfully);
  }

/*****************************************************************************/
/****************************** Resize image *********************************/
/*****************************************************************************/
// Return 0 on success
// Return != 0 on error

static int Med_ResizeImage (struct Media *Media,
                            const char PathFileOriginal[PATH_MAX + 1],
                            const char PathFileProcessed[PATH_MAX + 1])
  {
   char Command[256 + PATH_MAX * 2];
   int ReturnCode;

   snprintf (Command,sizeof (Command),
	     "convert %s -resize '%ux%u>' -quality %u %s",
             PathFileOriginal,
             Media->Width,
             Media->Height,
             Media->Quality,
             PathFileProcessed);
   ReturnCode = system (Command);
   if (ReturnCode == -1)
      Lay_ShowErrorAndExit ("Error when running command to process media.");

   ReturnCode = WEXITSTATUS(ReturnCode);

   return ReturnCode;
  }

/*****************************************************************************/
/************ Process original media generating processed media **************/
/*****************************************************************************/
// Return 0 on success
// Return != 0 on error

static int Med_GetFirstFrame (const char PathFileOriginal[PATH_MAX + 1],
                              const char PathFileProcessed[PATH_MAX + 1])
  {
   char Command[128 + PATH_MAX * 2];
   int ReturnCode;

   snprintf (Command,sizeof (Command),
	     "convert '%s[0]' %s",
             PathFileOriginal,
             PathFileProcessed);
   ReturnCode = system (Command);
   if (ReturnCode == -1)
      Lay_ShowErrorAndExit ("Error when running command to process media.");

   ReturnCode = WEXITSTATUS(ReturnCode);

   return ReturnCode;
  }

/*****************************************************************************/
/**************************** Get media from form ****************************/
/*****************************************************************************/

static void Med_GetAndProcessEmbedFromForm (const char *ParamURL,
                                            struct Media *Media)
  {
   extern const char Str_BIN_TO_BASE64URL[64 + 1];
   char *PtrHost   = NULL;
   char *PtrPath   = NULL;
   char *PtrParams = NULL;
   char *PtrCode   = NULL;
   size_t CodeLength;
   char *Code;
   enum
     {
      WRONG,	// Bad formed YouTube URL
      SHORT,	// youtu.be
      FULL,	// www.youtube.com/watch?
      EMBED,	// www.youtube.com/embed/
     } YouTube = WRONG;

   /***** Set media status *****/
   Media->Status = Med_STATUS_NONE;

   /***** Get embed URL from form *****/
   Usr_GetURLFromForm (ParamURL,Media);
   // Ale_ShowAlert (Ale_INFO,"DEBUG: Media->URL = '%s'",Media->URL);

   /***** Process URL trying to convert it to a YouTube embed URL *****/
   if (Media->URL)
      if (Media->URL[0])	// URL given by user is not empty
	{
	 /* Examples of valid YouTube URLs:
	    https://www.youtube.com/watch?v=xu9IbeF9CBw
	    https://www.youtube.com/watch?v=xu9IbeF9CBw&t=10
	    https://youtu.be/xu9IbeF9CBw
	    https://youtu.be/xu9IbeF9CBw?t=10
	    https://www.youtube.com/embed/xu9IbeF9CBw
	    https://www.youtube.com/embed/xu9IbeF9CBw?start=10
	 */
	 /***** Step 1: Skip scheme *****/
	 if      (!strncasecmp (Media->URL,"https://",8))	// URL starts by https://
	    PtrHost = &Media->URL[8];
	 else if (!strncasecmp (Media->URL,"http://" ,7))	// URL starts by http://
	    PtrHost = &Media->URL[7];
	 else if (!strncasecmp (Media->URL,"//"      ,2))	// URL starts by //
	    PtrHost = &Media->URL[2];
	 else
	    PtrHost = &Media->URL[0];

	 if (PtrHost[0])
	   {
	    /***** Step 2: Skip host *****/
	    // Ale_ShowAlert (Ale_INFO,"DEBUG: PtrHost = '%s'",PtrHost);
	    if      (!strncasecmp (PtrHost,"youtu.be/"       , 9))	// Host starts by youtu.be/
	      {
	       YouTube = SHORT;
	       PtrPath = &PtrHost[9];
	      }
	    else if (!strncasecmp (PtrHost,"www.youtube.com/",16))	// Host starts by www.youtube.com/
	      {
	       YouTube = FULL;
	       PtrPath = &PtrHost[16];
	      }
	    else if (!strncasecmp (PtrHost,"youtube.com/"    ,12))	// Host starts by youtube.com/
	      {
	       YouTube = FULL;
	       PtrPath = &PtrHost[12];
	      }

	    /* Check pointer to path */
	    if (PtrPath)
	      {
	       if (!PtrPath[0])
		  YouTube = WRONG;
	      }
	    else
	       YouTube = WRONG;

	    if (YouTube != WRONG)
	      {
	       // Ale_ShowAlert (Ale_INFO,"DEBUG: PtrPath = '%s'",PtrPath);
	       /***** Step 3: Skip path *****/
	       if (YouTube == FULL)
		 {
		  if      (!strncasecmp (PtrPath,"watch?",6))	// Path starts by watch?
		     PtrParams = &PtrPath[6];
		  else if (!strncasecmp (PtrPath,"embed/",6))	// Path starts by embed/
		    {
		     YouTube = EMBED;
		     PtrParams = &PtrPath[6];
		    }
		  else
		     YouTube = WRONG;
		 }
	       else
		  PtrParams = &PtrPath[0];

	       /* Check pointer to params */
	       if (PtrParams)
		 {
		  if (!PtrParams[0])
		     YouTube = WRONG;
		 }
	       else
		  YouTube = WRONG;

	       if (YouTube != WRONG)
		 {
		  // Ale_ShowAlert (Ale_INFO,"DEBUG: PtrParams = '%s'",PtrParams);
		  /***** Step 4: Search for video code *****/
		  switch (YouTube)
		    {
		     case SHORT:
			PtrCode = PtrParams;
			break;
		     case FULL:
			/* Search for v= */
			PtrCode = strcasestr (PtrPath,"v=");
			if (PtrCode)
			   PtrCode += 2;
			break;
		     case EMBED:
			PtrCode = PtrParams;
			break;
		     default:
			PtrCode = NULL;
			break;
		    }

		  /* Check pointer to code */
		  if (PtrCode)
		    {
		     if (!PtrCode[0])
			YouTube = WRONG;
		    }
		  else
		     YouTube = WRONG;

		  if (YouTube != WRONG)
		    {
		     // Ale_ShowAlert (Ale_INFO,"DEBUG: PtrCode = '%s'",PtrCode);
		     /***** Step 5: Get video code *****/
		     CodeLength = strspn (PtrCode,Str_BIN_TO_BASE64URL);
		     if (CodeLength > 0)
		       {
			/* Allocate space for YouTube code */
			if ((Code = (char *) malloc (CodeLength + 1)) == NULL)
			   Lay_ShowErrorAndExit ("Error allocating memory for YouTube code.");

			/* Copy code */
			strncpy (Code,PtrCode,CodeLength);
			Code[CodeLength] = '\0';
			// Ale_ShowAlert (Ale_INFO,"DEBUG: Code = '%s'",Code);

			/* Overwrite current URL with the embed URL */
			Med_FreeMediaURL (Media);
			if (asprintf (&Media->URL,"https://www.youtube.com/embed/%s",
				      Code) < 0)
			   Lay_NotEnoughMemoryExit ();
			if (strlen (Media->URL) <= Cns_MAX_BYTES_WWW)
			  {
			   /***** Success! *****/
			   // Ale_ShowAlert (Ale_INFO,"DEBUG: Media->URL = '%s'",Media->URL);
			   Media->Type = Med_YOUTUBE;
			   Media->Status = Med_PROCESSED;
			  }
			else
			   Med_FreeMediaURL (Media);

			/* Free YouTube code */
			free ((void *) Code);
		       }
		    }
		 }
	      }
	   }
	}
  }

/*****************************************************************************/
/**** Move temporary processed media file to definitive private directory ****/
/*****************************************************************************/

void Med_MoveMediaToDefinitiveDir (struct Media *Media)
  {
   char PathMedPrivTmp[PATH_MAX + 1];
   char PathMedPriv[PATH_MAX + 1];

   /***** Check trivial cases *****/
   if (Media->Type == Med_TYPE_NONE)
      Lay_ShowErrorAndExit ("Wrong media type.");
   if (Media->Type == Med_YOUTUBE)
      return;	// Nothing to do with files

   /***** Build temporary path *****/
   snprintf (PathMedPrivTmp,sizeof (PathMedPrivTmp),
	     "%s/%s/%s",
	     Cfg_PATH_SWAD_PRIVATE,Cfg_FOLDER_MEDIA,Cfg_FOLDER_IMG_TMP);

   /***** Create private subdirectory for media if it does not exist *****/
   snprintf (PathMedPriv,sizeof (PathMedPriv),
	     "%s/%s/%c%c",
	     Cfg_PATH_SWAD_PRIVATE,Cfg_FOLDER_MEDIA,
	     Media->Name[0],
	     Media->Name[1]);
   Fil_CreateDirIfNotExists (PathMedPriv);

   /***** Move files *****/
   switch (Media->Type)
     {
      case Med_JPG:
	 /* Move JPG */
	 if (!Med_MoveTmpFileToDefDir (Media,PathMedPrivTmp,PathMedPriv,
	                               Med_Extensions[Med_JPG]))
	    return;			// Fail
	 break;
      case Med_GIF:
	 /* Move PNG */
	 if (!Med_MoveTmpFileToDefDir (Media,PathMedPrivTmp,PathMedPriv,
	                               "png"))
	    return;			// Fail
	 /* Move GIF */
	 if (!Med_MoveTmpFileToDefDir (Media,PathMedPrivTmp,PathMedPriv,
	                               Med_Extensions[Med_GIF]))
	    return;			// Fail
	 break;
      case Med_MP4:
      case Med_WEBM:
      case Med_OGG:
	 /* Move MP4 or WEBM or OGG */
	 if (!Med_MoveTmpFileToDefDir (Media,PathMedPrivTmp,PathMedPriv,
	                               Med_Extensions[Media->Type]))
	    return;			// Fail
	 break;
      default:
	 Lay_ShowErrorAndExit ("Wrong media type.");
	 break;
     }

   Media->Status = Med_FILE_MOVED;	// Success
  }

/*****************************************************************************/
/******* Move temporary processed file to definitive private directory *******/
/*****************************************************************************/
// Return true on success
// Return false on error

static bool Med_MoveTmpFileToDefDir (struct Media *Media,
				     const char PathMedPrivTmp[PATH_MAX + 1],
				     const char PathMedPriv[PATH_MAX + 1],
				     const char *Extension)
  {
   char PathFileTmp[PATH_MAX + 1];	// Full name of temporary processed file
   char PathFile[PATH_MAX + 1];	// Full name of definitive processed file

   /***** Temporary processed media file *****/
   snprintf (PathFileTmp,sizeof (PathFileTmp),
	     "%s/%s.%s",
	     PathMedPrivTmp,Media->Name,Extension);

   /***** Definitive processed media file *****/
   snprintf (PathFile,sizeof (PathFile),
	     "%s/%s.%s",
	     PathMedPriv,Media->Name,Extension);

   /***** Move JPG file *****/
   if (rename (PathFileTmp,PathFile))	// Fail
     {
      Ale_ShowAlert (Ale_ERROR,"Can not move file.");
      return false;
     }

   return true;				// Success
  }

/*****************************************************************************/
/****** Show a user uploaded media (in a test question, timeline, etc.) ******/
/*****************************************************************************/

void Med_ShowMedia (struct Media *Media,
                    const char *ClassContainer,const char *ClassMedia)
  {
   bool PutLink;
   char PathMedPriv[PATH_MAX + 1];

   /***** If no media to show ==> nothing to do *****/
   if (Media->Status != Med_STORED_IN_DB)
      return;
   if (Media->Type == Med_TYPE_NONE)
      return;

   /***** Start media container *****/
   fprintf (Gbl.F.Out,"<div class=\"%s",ClassContainer);
   if (Media->Type == Med_YOUTUBE)
      fprintf (Gbl.F.Out," MED_VIDEO_CONT");
   fprintf (Gbl.F.Out,"\">");

   if (Media->Type == Med_YOUTUBE)
      /***** Show media *****/
      Med_ShowYoutube (Media,ClassMedia);
   else	// Uploaded file
     {
      /***** If no media to show ==> nothing to do *****/
      if (!Media->Name)
	 return;
      if (!Media->Name[0])
	 return;

      /***** Start optional link to external URL *****/
      PutLink = false;
      if (Media->URL)
	 if (Media->URL[0])
	    PutLink = true;
      if (PutLink)
	 fprintf (Gbl.F.Out,"<a href=\"%s\" target=\"_blank\">",Media->URL);

      /***** Create a temporary public directory used to show the media *****/
      Brw_CreateDirDownloadTmp ();

      /***** Build path to private directory with the media *****/
      snprintf (PathMedPriv,sizeof (PathMedPriv),
		"%s/%s/%c%c",
		Cfg_PATH_SWAD_PRIVATE,Cfg_FOLDER_MEDIA,
		Media->Name[0],
		Media->Name[1]);

      /***** Show media *****/
      switch (Media->Type)
	{
	 case Med_JPG:
	    Med_ShowJPG (Media,PathMedPriv,ClassMedia);
	    break;
	 case Med_GIF:
	    Med_ShowGIF (Media,PathMedPriv,ClassMedia);
	    break;
	 case Med_MP4:
	 case Med_WEBM:
	 case Med_OGG:
	    Med_ShowVideo (Media,PathMedPriv,ClassMedia);
	    break;
	 default:
	    break;
	}

      /***** End optional link to external URL *****/
      if (PutLink)
	 fprintf (Gbl.F.Out,"</a>");
     }

   /***** End media container *****/
   fprintf (Gbl.F.Out,"</div>");
  }

/*****************************************************************************/
/************************** Show a user uploaded JPG *************************/
/*****************************************************************************/

static void Med_ShowJPG (struct Media *Media,
			 const char PathMedPriv[PATH_MAX + 1],
			 const char *ClassMedia)
  {
   extern const char *Txt_File_not_found;
   char FileNameMediaPriv[NAME_MAX + 1];
   char FullPathMediaPriv[PATH_MAX + 1];
   char URLTmp[PATH_MAX + 1];
   char URL_JPG[PATH_MAX + 1];

   /***** Build private path to JPG *****/
   snprintf (FileNameMediaPriv,sizeof (FileNameMediaPriv),
	     "%s.%s",
	     Media->Name,Med_Extensions[Med_JPG]);
   snprintf (FullPathMediaPriv,sizeof (FullPathMediaPriv),
	     "%s/%s",
	     PathMedPriv,FileNameMediaPriv);

   /***** Check if private media file exists *****/
   if (Fil_CheckIfPathExists (FullPathMediaPriv))
     {
      /***** Create symbolic link from temporary public directory to private file
	     in order to gain access to it for showing/downloading *****/
      Brw_CreateTmpPublicLinkToPrivateFile (FullPathMediaPriv,FileNameMediaPriv);

      /***** Build temporary public URL *****/
      snprintf (URLTmp,sizeof (URLTmp),
		"%s/%s/%s",
		Cfg_URL_SWAD_PUBLIC,Cfg_FOLDER_FILE_BROWSER_TMP,
		Gbl.FileBrowser.TmpPubDir);

      /***** Create URL pointing to symbolic link *****/
      snprintf (URL_JPG,sizeof (URL_JPG),
		"%s/%s",
		URLTmp,FileNameMediaPriv);

      /***** Show media *****/
      fprintf (Gbl.F.Out,"<img src=\"%s\" class=\"%s\" alt=\"\"",URL_JPG,ClassMedia);
      if (Media->Title)
	 if (Media->Title[0])
	    fprintf (Gbl.F.Out," title=\"%s\"",Media->Title);
      fprintf (Gbl.F.Out," lazyload=\"on\" />");	// Lazy load of the media
     }
   else
      fprintf (Gbl.F.Out,"%s",Txt_File_not_found);
  }

/*****************************************************************************/
/************************** Show a user uploaded GIF *************************/
/*****************************************************************************/

static void Med_ShowGIF (struct Media *Media,
			 const char PathMedPriv[PATH_MAX + 1],
			 const char *ClassMedia)
  {
   extern const char *Txt_File_not_found;
   char FileNameMediaPriv[NAME_MAX + 1];
   char FullPathMediaPriv[PATH_MAX + 1];
   char URLTmp[PATH_MAX + 1];
   char URL_GIF[PATH_MAX + 1];
   char URL_PNG[PATH_MAX + 1];

   /***** Build private path to animated GIF image *****/
   snprintf (FileNameMediaPriv,sizeof (FileNameMediaPriv),
	     "%s.%s",
	     Media->Name,Med_Extensions[Med_GIF]);
   snprintf (FullPathMediaPriv,sizeof (FullPathMediaPriv),
	     "%s/%s",
	     PathMedPriv,FileNameMediaPriv);

   /***** Check if private media file exists *****/
   if (Fil_CheckIfPathExists (FullPathMediaPriv))
     {
      /***** Create symbolic link from temporary public directory to private file
	     in order to gain access to it for showing/downloading *****/
      Brw_CreateTmpPublicLinkToPrivateFile (FullPathMediaPriv,FileNameMediaPriv);

      /***** Build temporary public URL *****/
      snprintf (URLTmp,sizeof (URLTmp),
		"%s/%s/%s",
		Cfg_URL_SWAD_PUBLIC,Cfg_FOLDER_FILE_BROWSER_TMP,
		Gbl.FileBrowser.TmpPubDir);

      /***** Create URL pointing to symbolic link *****/
      snprintf (URL_GIF,sizeof (URL_GIF),
		"%s/%s",
		URLTmp,FileNameMediaPriv);

      /***** Build private path to static PNG image *****/
      snprintf (FileNameMediaPriv,sizeof (FileNameMediaPriv),
		"%s.png",
		Media->Name);
      snprintf (FullPathMediaPriv,sizeof (FullPathMediaPriv),
		"%s/%s",
		PathMedPriv,FileNameMediaPriv);

      /***** Check if private media file exists *****/
      if (Fil_CheckIfPathExists (FullPathMediaPriv))
	{
	 /***** Create symbolic link from temporary public directory to private file
		in order to gain access to it for showing/downloading *****/
	 Brw_CreateTmpPublicLinkToPrivateFile (FullPathMediaPriv,FileNameMediaPriv);

	 /***** Create URL pointing to symbolic link *****/
	 snprintf (URL_PNG,sizeof (URL_PNG),
		   "%s/%s",
		   URLTmp,FileNameMediaPriv);

	 /***** Show static PNG and animated GIF *****/
	 fprintf (Gbl.F.Out,"<div class=\"MED_PLAY\""
			    " onmouseover=\"toggleOnGIF(this,'%s');\""
			    " onmouseout=\"toggleOffGIF(this,'%s');\">",
		  URL_GIF,
		  URL_PNG);

	 /* Image */
	 fprintf (Gbl.F.Out,"<img src=\"%s\" class=\"%s\" alt=\"\"",

		  URL_PNG,
		  ClassMedia);
	 if (Media->Title)
	    if (Media->Title[0])
	       fprintf (Gbl.F.Out," title=\"%s\"",Media->Title);
	 fprintf (Gbl.F.Out," lazyload=\"on\" />");	// Lazy load of the media

	 /* Overlay with GIF label */
	 fprintf (Gbl.F.Out,"<span class=\"MED_PLAY_ICO\">"
			    "GIF"
			    "</span>");

	 fprintf (Gbl.F.Out,"</div>");
	}
      else
	 fprintf (Gbl.F.Out,"%s",Txt_File_not_found);
     }
   else
      fprintf (Gbl.F.Out,"%s",Txt_File_not_found);
  }

/*****************************************************************************/
/************************ Show a user uploaded video *************************/
/*****************************************************************************/

static void Med_ShowVideo (struct Media *Media,
			   const char PathMedPriv[PATH_MAX + 1],
			   const char *ClassMedia)
  {
   extern const char *Txt_File_not_found;
   char FileNameMediaPriv[NAME_MAX + 1];
   char FullPathMediaPriv[PATH_MAX + 1];
   char URLTmp[PATH_MAX + 1];
   char URL_Video[PATH_MAX + 1];

   /***** Build private path to video *****/
   snprintf (FileNameMediaPriv,sizeof (FileNameMediaPriv),
	     "%s.%s",
	     Media->Name,Med_Extensions[Media->Type]);
   snprintf (FullPathMediaPriv,sizeof (FullPathMediaPriv),
	     "%s/%s",
	     PathMedPriv,FileNameMediaPriv);

   /***** Check if private media file exists *****/
   if (Fil_CheckIfPathExists (FullPathMediaPriv))
     {
      /***** Create symbolic link from temporary public directory to private file
	     in order to gain access to it for showing/downloading *****/
      Brw_CreateTmpPublicLinkToPrivateFile (FullPathMediaPriv,FileNameMediaPriv);

      /***** Build temporary public URL *****/
      snprintf (URLTmp,sizeof (URLTmp),
		"%s/%s/%s",
		Cfg_URL_SWAD_PUBLIC,Cfg_FOLDER_FILE_BROWSER_TMP,
		Gbl.FileBrowser.TmpPubDir);

      /***** Create URL pointing to symbolic link *****/
      snprintf (URL_Video,sizeof (URL_Video),
		"%s/%s",
		URLTmp,FileNameMediaPriv);

      /***** Show media *****/
      fprintf (Gbl.F.Out,"<video src=\"%s\""
			 " preload=\"metadata\" controls=\"controls\""
			 " class=\"%s\"",
	       URL_Video,ClassMedia);
      if (Media->Title)
	 if (Media->Title[0])
	    fprintf (Gbl.F.Out," title=\"%s\"",Media->Title);
      fprintf (Gbl.F.Out," lazyload=\"on\">"	// Lazy load of the media
                         "Your browser does not support HTML5 video."
	                 "</video>");
     }
   else
      fprintf (Gbl.F.Out,"%s",Txt_File_not_found);
  }

/*****************************************************************************/
/*************************** Show an embed media *****************************/
/*****************************************************************************/

static void Med_ShowYoutube (struct Media *Media,
			     const char *ClassMedia)
  {
   /***** Check if embed URL exists *****/
   if (Media->URL[0])	// Embed URL
     {
      /***** Show linked external media *****/
      // Example of code give by YouTube:
      // <iframe width="560" height="315"
      // 	src="https://www.youtube.com/embed/xu9IbeF9CBw"
      // 	frameborder="0"
      // 	allow="accelerometer; autoplay; encrypted-media;
      // 	gyroscope; picture-in-picture" allowfullscreen>
      // </iframe>
      fprintf (Gbl.F.Out,"<iframe src=\"%s\""
			 " frameborder=\"0\""
			 " allow=\"accelerometer; autoplay; encrypted-media;"
			 " gyroscope; picture-in-picture\""
			 " allowfullscreen=\"allowfullscreen\""
			 " class=\"%s\"",
	       Media->URL,ClassMedia);
      if (Media->Title)
	 if (Media->Title[0])
	    fprintf (Gbl.F.Out," title=\"%s\"",Media->Title);
      fprintf (Gbl.F.Out,">"
	                 "</iframe>");
     }
  }

/*****************************************************************************/
/*** Remove private files with an image/video, given the image/video name ****/
/*****************************************************************************/

void Med_RemoveMediaFilesFromAllRows (unsigned NumMedia,MYSQL_RES *mysql_res)
  {
   unsigned NumMed;

   /***** Go over result removing media files *****/
   for (NumMed = 0;
	NumMed < NumMedia;
	NumMed++)
      Med_RemoveMediaFilesFromRow (mysql_res);
  }

void Med_RemoveMediaFilesFromRow (MYSQL_RES *mysql_res)
  {
   MYSQL_ROW row;

   /***** Get media name (row[0]) and type (row[1]) *****/
   row = mysql_fetch_row (mysql_res);

   /***** Remove image file *****/
   Med_RemoveMediaFiles (row[0],Med_GetTypeFromStrInDB (row[1]));
  }

void Med_RemoveMediaFiles (const char *Name,Med_Type_t Type)
  {
   char PathMedPriv[PATH_MAX + 1];
   char FullPathMediaPriv[PATH_MAX + 1];

   /***** Trivial cases *****/
   if (Type == Med_TYPE_NONE)
      Lay_ShowErrorAndExit ("Wrong media type.");
   if (Type == Med_YOUTUBE)
      return;

   /***** Build path to private directory with the media *****/
   snprintf (PathMedPriv,sizeof (PathMedPriv),
	     "%s/%s/%c%c",
	     Cfg_PATH_SWAD_PRIVATE,Cfg_FOLDER_MEDIA,
	     Name[0],
	     Name[1]);

   if (Name[0])
     {
      switch (Type)
        {
         case Med_JPG:
	    /***** Remove private JPG file *****/
	    snprintf (FullPathMediaPriv,sizeof (FullPathMediaPriv),
		      "%s/%s.%s",
		      PathMedPriv,Name,Med_Extensions[Med_JPG]);
	    unlink (FullPathMediaPriv);

	    break;
         case Med_GIF:
	    /***** Remove private GIF file *****/
	    snprintf (FullPathMediaPriv,sizeof (FullPathMediaPriv),
		      "%s/%s.%s",
		      PathMedPriv,Name,Med_Extensions[Med_GIF]);
	    unlink (FullPathMediaPriv);

	    /***** Remove private PNG file *****/
	    snprintf (FullPathMediaPriv,sizeof (FullPathMediaPriv),
		      "%s/%s.png",
		      PathMedPriv,Name);
	    unlink (FullPathMediaPriv);

	    break;
         case Med_MP4:
         case Med_WEBM:
         case Med_OGG:
	    /***** Remove private video file *****/
	    snprintf (FullPathMediaPriv,sizeof (FullPathMediaPriv),
		      "%s/%s.%s",
		      PathMedPriv,Name,Med_Extensions[Type]);
	    unlink (FullPathMediaPriv);

	    break;
         default:
            break;
        }

      // Public links are removed automatically after a period
     }
  }

/*****************************************************************************/
/************************ Get media type from string *************************/
/*****************************************************************************/

Med_Type_t Med_GetTypeFromStrInDB (const char *Str)
  {
   Med_Type_t Type;

   for (Type = (Med_Type_t) 0;
        Type < Med_NUM_TYPES;
        Type++)
      if (!strcasecmp (Str,Med_StringsTypeDB[Type]))
         return Type;

   return Med_TYPE_NONE;
  }

/*****************************************************************************/
/************************ Get media type from extension **********************/
/*****************************************************************************/

static Med_Type_t Med_GetTypeFromExtAndMIME (const char *Extension,
                                             const char *MIMEType)
  {
   /***** Extensions and MIME types allowed to convert to JPG *****/
   if (!strcasecmp (Extension,"jpg" ) ||
       !strcasecmp (Extension,"jpeg") ||
       !strcasecmp (Extension,"png" ) ||
       !strcasecmp (Extension,"tif" ) ||
       !strcasecmp (Extension,"tiff"))
      if (!strcmp (MIMEType,"image/jpeg"              ) ||
          !strcmp (MIMEType,"image/pjpeg"             ) ||
          !strcmp (MIMEType,"image/png"               ) ||
          !strcmp (MIMEType,"image/x-png"             ) ||
	  !strcmp (MIMEType,"image/tiff"              ) ||
          !strcmp (MIMEType,"application/octet-stream") ||
	  !strcmp (MIMEType,"application/octetstream" ) ||
	  !strcmp (MIMEType,"application/octet"       ))
	 return Med_JPG;

   /***** Extensions and MIME types allowed to convert to GIF *****/
   if (!strcasecmp (Extension,"gif"))
      if (!strcmp (MIMEType,"image/gif"               ) ||
          !strcmp (MIMEType,"application/octet-stream") ||
	  !strcmp (MIMEType,"application/octetstream" ) ||
	  !strcmp (MIMEType,"application/octet"       ))
	 return Med_GIF;

   /***** Extensions and MIME types allowed to convert to MP4 *****/
   if (!strcasecmp (Extension,"mp4"))
      if (!strcmp (MIMEType,"video/mp4"               ) ||
          !strcmp (MIMEType,"application/octet-stream") ||
	  !strcmp (MIMEType,"application/octetstream" ) ||
	  !strcmp (MIMEType,"application/octet"       ))
	 return Med_MP4;

   /***** Extensions and MIME types allowed to convert to WEBM *****/
   if (!strcasecmp (Extension,"webm"))
      if (!strcmp (MIMEType,"video/webm"              ) ||
          !strcmp (MIMEType,"application/octet-stream") ||
	  !strcmp (MIMEType,"application/octetstream" ) ||
	  !strcmp (MIMEType,"application/octet"       ))
	 return Med_WEBM;

   /***** Extensions and MIME types allowed to convert to OGG *****/
   if (!strcasecmp (Extension,"ogg"))
      if (!strcmp (MIMEType,"video/ogg"               ) ||
          !strcmp (MIMEType,"application/octet-stream") ||
	  !strcmp (MIMEType,"application/octetstream" ) ||
	  !strcmp (MIMEType,"application/octet"       ))
	 return Med_OGG;

   return Med_TYPE_NONE;
  }

/*****************************************************************************/
/*************** Get string media type in database from type *****************/
/*****************************************************************************/

const char *Med_GetStringTypeForDB (Med_Type_t Type)
  {
   /***** Check if type is out of valid range *****/
   if (Type > (Med_Type_t) (Med_NUM_TYPES - 1))
      return Med_StringsTypeDB[Med_TYPE_NONE];

   /***** Get string from type *****/
   return Med_StringsTypeDB[Type];
  }