// swad_centre_config.c: configuration of current centre

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
#include <unistd.h>		// For unlink

#include "swad_centre.h"
#include "swad_database.h"
#include "swad_figure_cache.h"
#include "swad_form.h"
#include "swad_global.h"
#include "swad_help.h"
#include "swad_hierarchy.h"
#include "swad_hierarchy_config.h"
#include "swad_HTML.h"
#include "swad_logo.h"
#include "swad_place.h"

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/***************************** Private constants *****************************/
/*****************************************************************************/

// Centre photo will be saved with:
// - maximum width of Ctr_PHOTO_SAVED_MAX_HEIGHT
// - maximum height of Ctr_PHOTO_SAVED_MAX_HEIGHT
// - maintaining the original aspect ratio (aspect ratio recommended: 3:2)
#define Ctr_RECOMMENDED_ASPECT_RATIO	"3:2"
#define Ctr_PHOTO_SAVED_MAX_WIDTH	768
#define Ctr_PHOTO_SAVED_MAX_HEIGHT	768
#define Ctr_PHOTO_SAVED_QUALITY		 90	// 1 to 100

/*****************************************************************************/
/******************************* Private types *******************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Private variables *****************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Private prototypes ****************************/
/*****************************************************************************/

static void CtrCfg_Configuration (bool PrintView);
static void CtrCfg_PutIconsCtrConfig (__attribute__((unused)) void *Args);
static void CtrCfg_PutIconToChangePhoto (void);
static void CtrCfg_Title (bool PutLink);
static void CtrCfg_Map (void);
static void CtrCfg_Latitude (void);
static void CtrCfg_Longitude (void);
static void CtrCfg_Altitude (void);
static void CtrCfg_Photo (bool PrintView,bool PutForm,bool PutLink,
			  const char PathPhoto[PATH_MAX + 1]);
static void CtrCfg_GetPhotoAttr (long CtrCod,char **PhotoAttribution);
static void CtrCfg_FreePhotoAttr (char **PhotoAttribution);
static void CtrCfg_Institution (bool PrintView,bool PutForm);
static void CtrCfg_FullName (bool PutForm);
static void CtrCfg_ShrtName (bool PutForm);
static void CtrCfg_Place (bool PutForm);
static void CtrCfg_WWW (bool PrintView,bool PutForm);
static void CtrCfg_Shortcut (bool PrintView);
static void CtrCfg_QR (void);
static void CtrCfg_NumUsrs (void);
static void CtrCfg_NumDegs (void);
static void CtrCfg_NumCrss (void);

static void CtrCfg_UpdateCtrInsDB (long CtrCod,long InsCod);
static void CtrCfg_UpdateCtrCoordinateDB (long CtrCod,
					  const char *CoordField,double NewCoord);

/*****************************************************************************/
/****************** Show information of the current centre *******************/
/*****************************************************************************/

void CtrCfg_ShowConfiguration (void)
  {
   CtrCfg_Configuration (false);

   /***** Show help to enrol me *****/
   Hlp_ShowHelpWhatWouldYouLikeToDo ();
  }

/*****************************************************************************/
/****************** Print information of the current centre ******************/
/*****************************************************************************/

void CtrCfg_PrintConfiguration (void)
  {
   CtrCfg_Configuration (true);
  }

/*****************************************************************************/
/******************* Information of the current centre ***********************/
/*****************************************************************************/

static void CtrCfg_Configuration (bool PrintView)
  {
   extern const char *Hlp_CENTRE_Information;
   bool PutLink;
   bool PutFormIns;
   bool PutFormName;
   bool PutFormPlc;
   bool PutFormCoor;
   bool PutFormWWW;
   bool PutFormPhoto;
   bool MapIsAvailable;
   char PathPhoto[PATH_MAX + 1];
   bool PhotoExists;

   /***** Trivial check *****/
   if (Gbl.Hierarchy.Ctr.CtrCod <= 0)	// No centre selected
      return;

   /***** Initializations *****/
   PutLink      = !PrintView && Gbl.Hierarchy.Ctr.WWW[0];
   PutFormIns   = !PrintView && Gbl.Usrs.Me.Role.Logged == Rol_SYS_ADM;
   PutFormName  = !PrintView && Gbl.Usrs.Me.Role.Logged >= Rol_INS_ADM;
   PutFormPlc   =
   PutFormCoor  =
   PutFormWWW   =
   PutFormPhoto = !PrintView && Gbl.Usrs.Me.Role.Logged >= Rol_CTR_ADM;

   /***** Begin box *****/
   if (PrintView)
      Box_BoxBegin (NULL,NULL,
                    NULL,NULL,
		    NULL,Box_NOT_CLOSABLE);
   else
      Box_BoxBegin (NULL,NULL,
                    CtrCfg_PutIconsCtrConfig,NULL,
		    Hlp_CENTRE_Information,Box_NOT_CLOSABLE);

   /***** Title *****/
   CtrCfg_Title (PutLink);

   /**************************** Left part ***********************************/
   HTM_DIV_Begin ("class=\"HIE_CFG_LEFT HIE_CFG_WIDTH\"");

   /***** Begin table *****/
   HTM_TABLE_BeginWidePadding (2);

   /***** Institution *****/
   CtrCfg_Institution (PrintView,PutFormIns);

   /***** Centre name *****/
   CtrCfg_FullName (PutFormName);
   CtrCfg_ShrtName (PutFormName);

   /***** Place *****/
   CtrCfg_Place (PutFormPlc);

   /***** Coordinates *****/
   if (PutFormCoor)
     {
      CtrCfg_Latitude ();
      CtrCfg_Longitude ();
      CtrCfg_Altitude ();
     }

   /***** Centre WWW *****/
   CtrCfg_WWW (PrintView,PutFormWWW);

   /***** Shortcut to the centre *****/
   CtrCfg_Shortcut (PrintView);

   if (PrintView)
      /***** QR code with link to the centre *****/
      CtrCfg_QR ();
   else
     {
      /***** Number of users who claim to belong to this centre,
             number of degrees,
             number of courses *****/
      CtrCfg_NumUsrs ();
      CtrCfg_NumDegs ();
      CtrCfg_NumCrss ();

      /***** Number of users in courses of this centre *****/
      HieCfg_NumUsrsInCrss (Hie_Lvl_CTR,Gbl.Hierarchy.Ctr.CtrCod,Rol_TCH);
      HieCfg_NumUsrsInCrss (Hie_Lvl_CTR,Gbl.Hierarchy.Ctr.CtrCod,Rol_NET);
      HieCfg_NumUsrsInCrss (Hie_Lvl_CTR,Gbl.Hierarchy.Ctr.CtrCod,Rol_STD);
      HieCfg_NumUsrsInCrss (Hie_Lvl_CTR,Gbl.Hierarchy.Ctr.CtrCod,Rol_UNK);
     }

   /***** End table *****/
   HTM_TABLE_End ();

   /***** End of left part *****/
   HTM_DIV_End ();

   /**************************** Right part **********************************/
   /***** Check map *****/
   MapIsAvailable = Ctr_GetIfMapIsAvailable (&Gbl.Hierarchy.Ctr);

   /***** Check photo *****/
   snprintf (PathPhoto,sizeof (PathPhoto),"%s/%02u/%u/%u.jpg",
	     Cfg_PATH_CTR_PUBLIC,
	     (unsigned) (Gbl.Hierarchy.Ctr.CtrCod % 100),
	     (unsigned)  Gbl.Hierarchy.Ctr.CtrCod,
	     (unsigned)  Gbl.Hierarchy.Ctr.CtrCod);
   PhotoExists = Fil_CheckIfPathExists (PathPhoto);

   if (MapIsAvailable || PhotoExists)
     {
      HTM_DIV_Begin ("class=\"HIE_CFG_RIGHT HIE_CFG_WIDTH\"");

      /***** Centre map *****/
      if (MapIsAvailable)
	 CtrCfg_Map ();

      /***** Centre photo *****/
      if (PhotoExists)
	 CtrCfg_Photo (PrintView,PutFormPhoto,PutLink,PathPhoto);

      HTM_DIV_End ();
     }

   /***** End box *****/
   Box_BoxEnd ();
  }

/*****************************************************************************/
/************ Put contextual icons in configuration of a centre **************/
/*****************************************************************************/

static void CtrCfg_PutIconsCtrConfig (__attribute__((unused)) void *Args)
  {
   /***** Put icon to print info about centre *****/
   Ico_PutContextualIconToPrint (ActPrnCtrInf,
				 NULL,NULL);

   /***** Put icon to view places *****/
   Plc_PutIconToViewPlaces ();

   if (Gbl.Usrs.Me.Role.Logged >= Rol_CTR_ADM)
      // Only centre admins, institution admins and system admins
      // have permission to upload logo and photo of the centre
     {
      /***** Put icon to upload logo of centre *****/
      Lgo_PutIconToChangeLogo (Hie_Lvl_CTR);

      /***** Put icon to upload photo of centre *****/
      CtrCfg_PutIconToChangePhoto ();
     }
  }

/*****************************************************************************/
/************* Put contextual icons to upload photo of centre ****************/
/*****************************************************************************/

static void CtrCfg_PutIconToChangePhoto (void)
  {
   extern const char *Txt_Change_photo;
   extern const char *Txt_Upload_photo;
   char PathPhoto[PATH_MAX + 1];
   bool PhotoExists;

   /***** Link to upload photo of centre *****/
   snprintf (PathPhoto,sizeof (PathPhoto),"%s/%02u/%u/%u.jpg",
	     Cfg_PATH_CTR_PUBLIC,
	     (unsigned) (Gbl.Hierarchy.Ctr.CtrCod % 100),
	     (unsigned)  Gbl.Hierarchy.Ctr.CtrCod,
	     (unsigned)  Gbl.Hierarchy.Ctr.CtrCod);
   PhotoExists = Fil_CheckIfPathExists (PathPhoto);
   Lay_PutContextualLinkOnlyIcon (ActReqCtrPho,NULL,
                                  NULL,NULL,
			          "camera.svg",
			          PhotoExists ? Txt_Change_photo :
				                Txt_Upload_photo);
  }

/*****************************************************************************/
/******************** Show title in centre configuration *********************/
/*****************************************************************************/

static void CtrCfg_Title (bool PutLink)
  {
   HieCfg_Title (PutLink,
		    Hie_Lvl_CTR,				// Logo scope
		    Gbl.Hierarchy.Ctr.CtrCod,		// Logo code
                    Gbl.Hierarchy.Ctr.ShrtName,		// Logo short name
		    Gbl.Hierarchy.Ctr.FullName,		// Logo full name
		    Gbl.Hierarchy.Ctr.WWW,		// Logo www
		    Gbl.Hierarchy.Ctr.FullName);	// Text full name
  }

/*****************************************************************************/
/****************************** Draw centre map ******************************/
/*****************************************************************************/

#define CtrCfg_MAP_CONTAINER_ID "ctr_mapid"

static void CtrCfg_Map (void)
  {
   /***** Leaflet CSS *****/
   Map_LeafletCSS ();

   /***** Leaflet script *****/
   Map_LeafletScript ();

   /***** Container for the map *****/
   HTM_DIV_Begin ("id=\"%s\"",CtrCfg_MAP_CONTAINER_ID);
   HTM_DIV_End ();

   /***** Script to draw the map *****/
   HTM_SCRIPT_Begin (NULL,NULL);

   /* Let's create a map with pretty Mapbox Streets tiles */
   Map_CreateMap (CtrCfg_MAP_CONTAINER_ID,&Gbl.Hierarchy.Ctr.Coord,16);

   /* Add Mapbox Streets tile layer to our map */
   Map_AddTileLayer ();

   /* Add marker */
   Map_AddMarker (&Gbl.Hierarchy.Ctr.Coord);

   /* Add popup */
   Map_AddPopup (Gbl.Hierarchy.Ctr.ShrtName,Gbl.Hierarchy.Ins.ShrtName,
		 true);	// Open

   HTM_SCRIPT_End ();
  }

/*****************************************************************************/
/************************** Edit centre coordinates **************************/
/*****************************************************************************/

static void CtrCfg_Latitude (void)
  {
   extern const char *Txt_Latitude;

   /***** Latitude *****/
   HTM_TR_Begin (NULL);

   /* Label */
   Frm_LabelColumn ("RT","Latitude",Txt_Latitude);

   /* Data */
   HTM_TD_Begin ("class=\"LB\"");
   Frm_StartForm (ActChgCtrLatCfg);
   HTM_INPUT_FLOAT ("Latitude",
		    -90.0,	// South Pole
		    90.0,	// North Pole
		    0.0,	// step="any"
		    Gbl.Hierarchy.Ctr.Coord.Latitude,false,
		    "class=\"INPUT_COORD\" required=\"required\"");
   Frm_EndForm ();
   HTM_TD_End ();

   HTM_TR_End ();
  }

static void CtrCfg_Longitude (void)
  {
   extern const char *Txt_Longitude;

   /***** Longitude *****/
   HTM_TR_Begin (NULL);

   /* Label */
   Frm_LabelColumn ("RT","Longitude",Txt_Longitude);

   /* Data */
   HTM_TD_Begin ("class=\"LB\"");
   Frm_StartForm (ActChgCtrLgtCfg);
   HTM_INPUT_FLOAT ("Longitude",
		    -180.0,	// West
		    180.0,	// East
		    0.0,	// step="any"
		    Gbl.Hierarchy.Ctr.Coord.Longitude,false,
		    "class=\"INPUT_COORD\" required=\"required\"");
   Frm_EndForm ();
   HTM_TD_End ();

   HTM_TR_End ();
  }

static void CtrCfg_Altitude (void)
  {
   extern const char *Txt_Altitude;

   /***** Altitude *****/
   HTM_TR_Begin (NULL);

   /* Label */
   Frm_LabelColumn ("RT","Altitude",Txt_Altitude);

   /* Data */
   HTM_TD_Begin ("class=\"LB\"");
   Frm_StartForm (ActChgCtrAltCfg);
   HTM_INPUT_FLOAT ("Altitude",
		    -413.0,	// Dead Sea shore
		    8848.0,	// Mount Everest
		    0.0,	// step="any"
		    Gbl.Hierarchy.Ctr.Coord.Altitude,false,
		    "class=\"INPUT_COORD\" required=\"required\"");
   Frm_EndForm ();
   HTM_TD_End ();

   HTM_TR_End ();
  }

/*****************************************************************************/
/***************************** Draw centre photo *****************************/
/*****************************************************************************/

static void CtrCfg_Photo (bool PrintView,bool PutForm,bool PutLink,
			  const char PathPhoto[PATH_MAX + 1])
  {
   char *PhotoAttribution = NULL;
   char *URL;
   char *Icon;

   /***** Trivial checks *****/
   if (!PathPhoto)
      return;
   if (!PathPhoto[0])
      return;

   /***** Get photo attribution *****/
   CtrCfg_GetPhotoAttr (Gbl.Hierarchy.Ctr.CtrCod,&PhotoAttribution);

   /***** Photo image *****/
   HTM_DIV_Begin ("class=\"DAT_SMALL CM\"");
   if (PutLink)
      HTM_A_Begin ("href=\"%s\" target=\"_blank\" class=\"DAT_N\"",
		   Gbl.Hierarchy.Ctr.WWW);
   if (asprintf (&URL,"%s/%02u/%u",
		 Cfg_URL_CTR_PUBLIC,
		 (unsigned) (Gbl.Hierarchy.Ctr.CtrCod % 100),
		 (unsigned) Gbl.Hierarchy.Ctr.CtrCod) < 0)
      Lay_NotEnoughMemoryExit ();
   if (asprintf (&Icon,"%u.jpg",
		 (unsigned) Gbl.Hierarchy.Ctr.CtrCod) < 0)
      Lay_NotEnoughMemoryExit ();
   HTM_IMG (URL,Icon,Gbl.Hierarchy.Ctr.FullName,
	    "class=\"%s\"",PrintView ? "CENTRE_PHOTO_PRINT CENTRE_PHOTO_WIDTH" :
				       "CENTRE_PHOTO_SHOW CENTRE_PHOTO_WIDTH");
   free (Icon);
   free (URL);
   if (PutLink)
      HTM_A_End ();
   HTM_DIV_End ();

   /****** Photo attribution ******/
   if (PutForm)
     {
      HTM_DIV_Begin ("class=\"CM\"");
      Frm_StartForm (ActChgCtrPhoAtt);
      HTM_TEXTAREA_Begin ("id=\"AttributionArea\" name=\"Attribution\" rows=\"3\""
			  " onchange=\"document.getElementById('%s').submit();return false;\"",
			  Gbl.Form.Id);
      if (PhotoAttribution)
	 HTM_Txt (PhotoAttribution);
      HTM_TEXTAREA_End ();
      Frm_EndForm ();
      HTM_DIV_End ();
     }
   else if (PhotoAttribution)
     {
      HTM_DIV_Begin ("class=\"ATTRIBUTION\"");
      HTM_Txt (PhotoAttribution);
      HTM_DIV_End ();
     }

   /****** Free memory used for photo attribution ******/
   CtrCfg_FreePhotoAttr (&PhotoAttribution);
  }

/*****************************************************************************/
/******************* Get photo attribution from database *********************/
/*****************************************************************************/

static void CtrCfg_GetPhotoAttr (long CtrCod,char **PhotoAttribution)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   size_t Length;

   /***** Free possible former photo attribution *****/
   CtrCfg_FreePhotoAttr (PhotoAttribution);

   /***** Get photo attribution from database *****/
   if (DB_QuerySELECT (&mysql_res,"can not get photo attribution",
		       "SELECT PhotoAttribution"
		       " FROM centres WHERE CtrCod=%ld",
		       CtrCod))
     {
      /* Get row */
      row = mysql_fetch_row (mysql_res);

      /* Get the attribution of the photo of the centre (row[0]) */
      if (row[0])
	 if (row[0][0])
	   {
	    Length = strlen (row[0]);
	    if (((*PhotoAttribution) = malloc (Length + 1)) == NULL)
               Lay_NotEnoughMemoryExit ();
	    Str_Copy (*PhotoAttribution,row[0],Length);
	   }
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/****************** Free memory used for photo attribution *******************/
/*****************************************************************************/

static void CtrCfg_FreePhotoAttr (char **PhotoAttribution)
  {
   if (*PhotoAttribution)
     {
      free (*PhotoAttribution);
      *PhotoAttribution = NULL;
     }
  }

/*****************************************************************************/
/***************** Show institution in centre configuration ******************/
/*****************************************************************************/

static void CtrCfg_Institution (bool PrintView,bool PutForm)
  {
   extern const char *Txt_Institution;
   unsigned NumIns;

   /***** Institution *****/
   HTM_TR_Begin (NULL);

   /* Label */
   Frm_LabelColumn ("RT",PutForm ? "OthInsCod" :
	                           NULL,
		    Txt_Institution);

   /* Data */
   HTM_TD_Begin ("class=\"DAT LB\"");
   if (PutForm)
     {
      /* Get list of institutions of the current country */
      Ins_GetBasicListOfInstitutions (Gbl.Hierarchy.Cty.CtyCod);

      /* Put form to select institution */
      Frm_StartForm (ActChgCtrInsCfg);
      HTM_SELECT_Begin (HTM_SUBMIT_ON_CHANGE,
			"id=\"OthInsCod\" name=\"OthInsCod\""
			" class=\"INPUT_SHORT_NAME\"");
      for (NumIns = 0;
	   NumIns < Gbl.Hierarchy.Inss.Num;
	   NumIns++)
	 HTM_OPTION (HTM_Type_LONG,&Gbl.Hierarchy.Inss.Lst[NumIns].InsCod,
		     Gbl.Hierarchy.Inss.Lst[NumIns].InsCod == Gbl.Hierarchy.Ins.InsCod,false,
	             "%s",Gbl.Hierarchy.Inss.Lst[NumIns].ShrtName);
      HTM_SELECT_End ();
      Frm_EndForm ();

      /* Free list of institutions */
      Ins_FreeListInstitutions ();
     }
   else	// I can not move centre to another institution
     {
      if (!PrintView)
	{
         Frm_StartFormGoTo (ActSeeInsInf);
         Ins_PutParamInsCod (Gbl.Hierarchy.Ins.InsCod);
	 HTM_BUTTON_SUBMIT_Begin (Hie_BuildGoToMsg (Gbl.Hierarchy.Ins.ShrtName),
				  "BT_LINK LT DAT",NULL);
	 Hie_FreeGoToMsg ();
	}
      Lgo_DrawLogo (Hie_Lvl_INS,Gbl.Hierarchy.Ins.InsCod,Gbl.Hierarchy.Ins.ShrtName,
		    20,"LM",true);
      HTM_NBSP ();
      HTM_Txt (Gbl.Hierarchy.Ins.FullName);
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
/************** Show centre full name in centre configuration ****************/
/*****************************************************************************/

static void CtrCfg_FullName (bool PutForm)
  {
   extern const char *Txt_Centre;

   HieCfg_FullName (PutForm,Txt_Centre,ActRenCtrFulCfg,
		       Gbl.Hierarchy.Ctr.FullName);
  }

/*****************************************************************************/
/************** Show centre short name in centre configuration ***************/
/*****************************************************************************/

static void CtrCfg_ShrtName (bool PutForm)
  {
   HieCfg_ShrtName (PutForm,ActRenCtrShoCfg,Gbl.Hierarchy.Ctr.ShrtName);
  }

/*****************************************************************************/
/**************** Show centre place in centre configuration ******************/
/*****************************************************************************/

static void CtrCfg_Place (bool PutForm)
  {
   extern const char *Txt_Place;
   extern const char *Txt_Another_place;
   struct Plc_Places Places;
   struct Plc_Place Plc;
   unsigned NumPlc;

   /***** Reset places context *****/
   Plc_ResetPlaces (&Places);

   /***** Get data of place *****/
   Plc.PlcCod = Gbl.Hierarchy.Ctr.PlcCod;
   Plc_GetDataOfPlaceByCod (&Plc);

   /***** Place *****/
   HTM_TR_Begin (NULL);

   /* Label */
   Frm_LabelColumn ("RT",PutForm ? "PlcCod" :
	                           NULL,
		    Txt_Place);

   /* Data */
   HTM_TD_Begin ("class=\"DAT LB\"");
   if (PutForm)
     {
      /* Get list of places of the current institution */
      Places.SelectedOrder = Plc_ORDER_BY_PLACE;
      Plc_GetListPlaces (&Places);

      /* Put form to select place */
      Frm_StartForm (ActChgCtrPlcCfg);
      HTM_SELECT_Begin (HTM_SUBMIT_ON_CHANGE,
			"name=\"PlcCod\" class=\"INPUT_SHORT_NAME\"");
      HTM_OPTION (HTM_Type_STRING,"0",
		  Gbl.Hierarchy.Ctr.PlcCod == 0,false,
		  "%s",Txt_Another_place);
      for (NumPlc = 0;
	   NumPlc < Places.Num;
	   NumPlc++)
	 HTM_OPTION (HTM_Type_LONG,&Places.Lst[NumPlc].PlcCod,
		     Places.Lst[NumPlc].PlcCod == Gbl.Hierarchy.Ctr.PlcCod,false,
		     "%s",Places.Lst[NumPlc].ShrtName);
      HTM_SELECT_End ();
      Frm_EndForm ();

      /* Free list of places */
      Plc_FreeListPlaces (&Places);
     }
   else	// I can not change centre place
      HTM_Txt (Plc.FullName);
   HTM_TD_End ();

   HTM_TR_End ();
  }

/*****************************************************************************/
/***************** Show centre WWW in centre configuration *******************/
/*****************************************************************************/

static void CtrCfg_WWW (bool PrintView,bool PutForm)
  {
   HieCfg_WWW (PrintView,PutForm,ActChgCtrWWWCfg,Gbl.Hierarchy.Ctr.WWW);
  }

/*****************************************************************************/
/*************** Show centre shortcut in centre configuration ****************/
/*****************************************************************************/

static void CtrCfg_Shortcut (bool PrintView)
  {
   HieCfg_Shortcut (PrintView,"ctr",Gbl.Hierarchy.Ctr.CtrCod);
  }

/*****************************************************************************/
/****************** Show centre QR in centre configuration *******************/
/*****************************************************************************/

static void CtrCfg_QR (void)
  {
   HieCfg_QR ("ctr",Gbl.Hierarchy.Ctr.CtrCod);
  }

/*****************************************************************************/
/*** Show number of users who claim to belong to centre in centre config. ****/
/*****************************************************************************/

static void CtrCfg_NumUsrs (void)
  {
   extern const char *Txt_Users_of_the_centre;

   /***** Number of users *****/
   HTM_TR_Begin (NULL);

   /* Label */
   Frm_LabelColumn ("RT",NULL,Txt_Users_of_the_centre);

   /* Data */
   HTM_TD_Begin ("class=\"DAT LB\"");
   HTM_Unsigned (Usr_GetCachedNumUsrsWhoClaimToBelongToCtr (&Gbl.Hierarchy.Ctr));
   HTM_TD_End ();

   HTM_TR_End ();
  }

/*****************************************************************************/
/************** Show number of degrees in centre configuration ***************/
/*****************************************************************************/

static void CtrCfg_NumDegs (void)
  {
   extern const char *Txt_Degrees;
   extern const char *Txt_Degrees_of_CENTRE_X;

   /***** Number of degrees *****/
   HTM_TR_Begin (NULL);

   /* Label */
   Frm_LabelColumn ("RT",NULL,Txt_Degrees);

   /* Data */
   HTM_TD_Begin ("class=\"LB\"");
   Frm_StartFormGoTo (ActSeeDeg);
   Ctr_PutParamCtrCod (Gbl.Hierarchy.Ctr.CtrCod);
   HTM_BUTTON_SUBMIT_Begin (Str_BuildStringStr (Txt_Degrees_of_CENTRE_X,
	                                        Gbl.Hierarchy.Ctr.ShrtName),
			    "BT_LINK DAT",NULL);
   Str_FreeString ();
   HTM_Unsigned (Deg_GetCachedNumDegsInCtr (Gbl.Hierarchy.Ctr.CtrCod));
   HTM_BUTTON_End ();
   Frm_EndForm ();
   HTM_TD_End ();

   HTM_TR_End ();
  }

/*****************************************************************************/
/************** Show number of courses in centre configuration ***************/
/*****************************************************************************/

static void CtrCfg_NumCrss (void)
  {
   extern const char *Txt_Courses;

   /***** Number of courses *****/
   HTM_TR_Begin (NULL);

   /* Label */
   Frm_LabelColumn ("RT",NULL,Txt_Courses);

   /* Data */
   HTM_TD_Begin ("class=\"DAT LB\"");
   HTM_Unsigned (Crs_GetCachedNumCrssInCtr (Gbl.Hierarchy.Ctr.CtrCod));
   HTM_TD_End ();

   HTM_TR_End ();
  }

/*****************************************************************************/
/*********** Show a form for sending a logo of the current centre ************/
/*****************************************************************************/

void CtrCfg_RequestLogo (void)
  {
   Lgo_RequestLogo (Hie_Lvl_CTR);
  }

/*****************************************************************************/
/***************** Receive the logo of the current centre ********************/
/*****************************************************************************/

void CtrCfg_ReceiveLogo (void)
  {
   Lgo_ReceiveLogo (Hie_Lvl_CTR);
  }

/*****************************************************************************/
/****************** Remove the logo of the current centre ********************/
/*****************************************************************************/

void CtrCfg_RemoveLogo (void)
  {
   Lgo_RemoveLogo (Hie_Lvl_CTR);
  }

/*****************************************************************************/
/*********** Show a form for sending a photo of the current centre ***********/
/*****************************************************************************/

void CtrCfg_RequestPhoto (void)
  {
   extern const char *The_ClassFormInBox[The_NUM_THEMES];
   extern const char *Txt_Photo;
   extern const char *Txt_Recommended_aspect_ratio;
   extern const char *Txt_Recommended_resolution;
   extern const char *Txt_XxY_pixels_or_higher;
   extern const char *Txt_File_with_the_photo;

   /***** Begin form to upload photo *****/
   Frm_StartForm (ActRecCtrPho);

   /***** Begin box *****/
   Box_BoxBegin (NULL,Txt_Photo,
                 NULL,NULL,
                 NULL,Box_NOT_CLOSABLE);

   /***** Write help message *****/
   Ale_ShowAlert (Ale_INFO,"%s: %s<br />"
		           "%s: %u&times;%u %s",
		  Txt_Recommended_aspect_ratio,
		  Ctr_RECOMMENDED_ASPECT_RATIO,
		  Txt_Recommended_resolution,
		  Ctr_PHOTO_SAVED_MAX_WIDTH,
		  Ctr_PHOTO_SAVED_MAX_HEIGHT,
		  Txt_XxY_pixels_or_higher);

   /***** Upload photo *****/
   HTM_LABEL_Begin ("class=\"%s\"",The_ClassFormInBox[Gbl.Prefs.Theme]);
   HTM_TxtColonNBSP (Txt_File_with_the_photo);
   HTM_INPUT_FILE (Fil_NAME_OF_PARAM_FILENAME_ORG,"image/*",
                   HTM_SUBMIT_ON_CHANGE,
                   NULL);
   HTM_LABEL_End ();

   /***** End box *****/
   Box_BoxEnd ();

   /***** End form *****/
   Frm_EndForm ();
  }

/*****************************************************************************/
/****************** Receive a photo of the current centre ********************/
/*****************************************************************************/

void CtrCfg_ReceivePhoto (void)
  {
   extern const char *Txt_Wrong_file_type;
   char Path[PATH_MAX + 1];
   struct Param *Param;
   char FileNameImgSrc[PATH_MAX + 1];
   char *PtrExtension;
   size_t LengthExtension;
   char MIMEType[Brw_MAX_BYTES_MIME_TYPE + 1];
   char PathFileImgTmp[PATH_MAX + 1];	// Full name (including path and .jpg) of the destination temporary file
   char PathFileImg[PATH_MAX + 1];	// Full name (including path and .jpg) of the destination file
   bool WrongType = false;
   char Command[1024 + PATH_MAX * 2];
   int ReturnCode;
   char ErrorMsg[256];

   /***** Copy in disk the file received *****/
   Param = Fil_StartReceptionOfFile (Fil_NAME_OF_PARAM_FILENAME_ORG,
                                     FileNameImgSrc,MIMEType);

   /* Check if the file type is image/ or application/octet-stream */
   if (strncmp (MIMEType,"image/",strlen ("image/")))
      if (strcmp (MIMEType,"application/octet-stream"))
	 if (strcmp (MIMEType,"application/octetstream"))
	    if (strcmp (MIMEType,"application/octet"))
	       WrongType = true;
   if (WrongType)
     {
      Ale_ShowAlert (Ale_WARNING,Txt_Wrong_file_type);
      return;
     }

   /***** Create private directories if not exist *****/
   /* Create private directory for images if it does not exist */
   Fil_CreateDirIfNotExists (Cfg_PATH_MEDIA_PRIVATE);

   /* Create temporary private directory for images if it does not exist */
   Fil_CreateDirIfNotExists (Cfg_PATH_MEDIA_TMP_PRIVATE);

   /* Get filename extension */
   if ((PtrExtension = strrchr (FileNameImgSrc,(int) '.')) == NULL)
     {
      Ale_ShowAlert (Ale_WARNING,Txt_Wrong_file_type);
      return;
     }
   LengthExtension = strlen (PtrExtension);
   if (LengthExtension < Fil_MIN_BYTES_FILE_EXTENSION ||
       LengthExtension > Fil_MAX_BYTES_FILE_EXTENSION)
     {
      Ale_ShowAlert (Ale_WARNING,Txt_Wrong_file_type);
      return;
     }

   /* End the reception of image in a temporary file */
   snprintf (PathFileImgTmp,sizeof (PathFileImgTmp),"%s/%s.%s",
             Cfg_PATH_MEDIA_TMP_PRIVATE,Gbl.UniqueNameEncrypted,PtrExtension);
   if (!Fil_EndReceptionOfFile (PathFileImgTmp,Param))
     {
      Ale_ShowAlert (Ale_WARNING,"Error copying file.");
      return;
     }

   /***** Creates public directories if not exist *****/
   Fil_CreateDirIfNotExists (Cfg_PATH_CTR_PUBLIC);
   snprintf (Path,sizeof (Path),"%s/%02u",
	     Cfg_PATH_CTR_PUBLIC,(unsigned) (Gbl.Hierarchy.Ctr.CtrCod % 100));
   Fil_CreateDirIfNotExists (Path);
   snprintf (Path,sizeof (Path),"%s/%02u/%u",
	     Cfg_PATH_CTR_PUBLIC,
	     (unsigned) (Gbl.Hierarchy.Ctr.CtrCod % 100),
	     (unsigned)  Gbl.Hierarchy.Ctr.CtrCod);
   Fil_CreateDirIfNotExists (Path);

   /***** Convert temporary file to public JPEG file *****/
   snprintf (PathFileImg,sizeof (PathFileImg),"%s/%02u/%u/%u.jpg",
	     Cfg_PATH_CTR_PUBLIC,
	     (unsigned) (Gbl.Hierarchy.Ctr.CtrCod % 100),
	     (unsigned)  Gbl.Hierarchy.Ctr.CtrCod,
	     (unsigned)  Gbl.Hierarchy.Ctr.CtrCod);

   /* Call to program that makes the conversion */
   snprintf (Command,sizeof (Command),
             "convert %s -resize '%ux%u>' -quality %u %s",
             PathFileImgTmp,
             Ctr_PHOTO_SAVED_MAX_WIDTH,
             Ctr_PHOTO_SAVED_MAX_HEIGHT,
             Ctr_PHOTO_SAVED_QUALITY,
             PathFileImg);
   ReturnCode = system (Command);
   if (ReturnCode == -1)
      Lay_ShowErrorAndExit ("Error when running command to process image.");

   /***** Write message depending on return code *****/
   ReturnCode = WEXITSTATUS(ReturnCode);
   if (ReturnCode != 0)
     {
      snprintf (ErrorMsg,sizeof (ErrorMsg),
	        "Image could not be processed successfully.<br />"
		"Error code returned by the program of processing: %d",
	        ReturnCode);
      Lay_ShowErrorAndExit (ErrorMsg);
     }

   /***** Remove temporary file *****/
   unlink (PathFileImgTmp);

   /***** Show the centre information again *****/
   CtrCfg_ShowConfiguration ();
  }

/*****************************************************************************/
/**************** Change the attribution of a centre photo *******************/
/*****************************************************************************/

void CtrCfg_ChangeCtrPhotoAttr (void)
  {
   char NewPhotoAttribution[Med_MAX_BYTES_ATTRIBUTION + 1];

   /***** Get parameters from form *****/
   /* Get the new photo attribution for the centre */
   Par_GetParToText ("Attribution",NewPhotoAttribution,Med_MAX_BYTES_ATTRIBUTION);

   /***** Update the table changing old attribution by new attribution *****/
   DB_QueryUPDATE ("can not update the photo attribution"
		   " of the current centre",
		   "UPDATE centres SET PhotoAttribution='%s'"
		   " WHERE CtrCod=%ld",
	           NewPhotoAttribution,Gbl.Hierarchy.Ctr.CtrCod);

   /***** Show the centre information again *****/
   CtrCfg_ShowConfiguration ();
  }

/*****************************************************************************/
/********************* Change the institution of a centre ********************/
/*****************************************************************************/

void CtrCfg_ChangeCtrIns (void)
  {
   extern const char *Txt_The_centre_X_already_exists;
   extern const char *Txt_The_centre_X_has_been_moved_to_the_institution_Y;
   struct Ins_Instit NewIns;

   /***** Get parameter with institution code *****/
   NewIns.InsCod = Ins_GetAndCheckParamOtherInsCod (1);

   /***** Check if institution has changed *****/
   if (NewIns.InsCod != Gbl.Hierarchy.Ctr.InsCod)
     {
      /***** Get data of new institution *****/
      Ins_GetDataOfInstitutionByCod (&NewIns);

      /***** Check if it already exists a centre with the same name in the new institution *****/
      if (Ctr_CheckIfCtrNameExistsInIns ("ShortName",
                                         Gbl.Hierarchy.Ctr.ShrtName,
                                         Gbl.Hierarchy.Ctr.CtrCod,
                                         NewIns.InsCod))
	 /***** Create warning message *****/
	 Ale_CreateAlert (Ale_WARNING,NULL,
	                  Txt_The_centre_X_already_exists,
		          Gbl.Hierarchy.Ctr.ShrtName);
      else if (Ctr_CheckIfCtrNameExistsInIns ("FullName",
                                              Gbl.Hierarchy.Ctr.FullName,
                                              Gbl.Hierarchy.Ctr.CtrCod,
                                              NewIns.InsCod))
	 /***** Create warning message *****/
	 Ale_CreateAlert (Ale_WARNING,NULL,
	                  Txt_The_centre_X_already_exists,
		          Gbl.Hierarchy.Ctr.FullName);
      else
	{
	 /***** Update institution in table of centres *****/
	 CtrCfg_UpdateCtrInsDB (Gbl.Hierarchy.Ctr.CtrCod,NewIns.InsCod);
	 Gbl.Hierarchy.Ctr.InsCod =
	 Gbl.Hierarchy.Ins.InsCod = NewIns.InsCod;

	 /***** Initialize again current course, degree, centre... *****/
	 Hie_InitHierarchy ();

	 /***** Create message to show the change made *****/
         Ale_CreateAlert (Ale_SUCCESS,NULL,
                          Txt_The_centre_X_has_been_moved_to_the_institution_Y,
		          Gbl.Hierarchy.Ctr.FullName,NewIns.FullName);
	}
     }
  }

/*****************************************************************************/
/******************* Update institution in table of centres ******************/
/*****************************************************************************/

static void CtrCfg_UpdateCtrInsDB (long CtrCod,long InsCod)
  {
   /***** Update institution in table of centres *****/
   DB_QueryUPDATE ("can not update the institution of a centre",
		   "UPDATE centres SET InsCod=%ld WHERE CtrCod=%ld",
                   InsCod,CtrCod);
  }

/*****************************************************************************/
/************************ Change the place of a centre ***********************/
/*****************************************************************************/

void CtrCfg_ChangeCtrPlc (void)
  {
   extern const char *Txt_The_place_of_the_centre_has_changed;
   long NewPlcCod;

   /***** Get parameter with place code *****/
   NewPlcCod = Plc_GetParamPlcCod ();

   /***** Update place in table of centres *****/
   Ctr_UpdateCtrPlcDB (Gbl.Hierarchy.Ctr.CtrCod,NewPlcCod);
   Gbl.Hierarchy.Ctr.PlcCod = NewPlcCod;

   /***** Write message to show the change made *****/
   Ale_ShowAlert (Ale_SUCCESS,Txt_The_place_of_the_centre_has_changed);

   /***** Show the form again *****/
   CtrCfg_ShowConfiguration ();
  }

/*****************************************************************************/
/*************** Change the name of a centre in configuration ****************/
/*****************************************************************************/

void CtrCfg_RenameCentreShort (void)
  {
   Ctr_RenameCentre (&Gbl.Hierarchy.Ctr,Cns_SHRT_NAME);
  }

void CtrCfg_RenameCentreFull (void)
  {
   Ctr_RenameCentre (&Gbl.Hierarchy.Ctr,Cns_FULL_NAME);
  }

/*****************************************************************************/
/********************** Change the latitude of a centre **********************/
/*****************************************************************************/

void CtrCfg_ChangeCtrLatitude (void)
  {
   char LatitudeStr[64];
   double NewLatitude;

   /***** Get latitude *****/
   Par_GetParToText ("Latitude",LatitudeStr,sizeof (LatitudeStr) - 1);
   NewLatitude = Map_GetLatitudeFromStr (LatitudeStr);

   /***** Update database changing old latitude by new latitude *****/
   CtrCfg_UpdateCtrCoordinateDB (Gbl.Hierarchy.Ctr.CtrCod,"Latitude",NewLatitude);
   Gbl.Hierarchy.Ctr.Coord.Latitude = NewLatitude;

   /***** Show the form again *****/
   CtrCfg_ShowConfiguration ();
  }

/*****************************************************************************/
/********************** Change the longitude of a centre **********************/
/*****************************************************************************/

void CtrCfg_ChangeCtrLongitude (void)
  {
   char LongitudeStr[64];
   double NewLongitude;

   /***** Get longitude *****/
   Par_GetParToText ("Longitude",LongitudeStr,sizeof (LongitudeStr) - 1);
   NewLongitude = Map_GetLongitudeFromStr (LongitudeStr);

   /***** Update database changing old longitude by new longitude *****/
   CtrCfg_UpdateCtrCoordinateDB (Gbl.Hierarchy.Ctr.CtrCod,"Longitude",NewLongitude);
   Gbl.Hierarchy.Ctr.Coord.Longitude = NewLongitude;

   /***** Show the form again *****/
   CtrCfg_ShowConfiguration ();
  }

/*****************************************************************************/
/********************** Change the latitude of a centre **********************/
/*****************************************************************************/

void CtrCfg_ChangeCtrAltitude (void)
  {
   char AltitudeStr[64];
   double NewAltitude;

   /***** Get altitude *****/
   Par_GetParToText ("Altitude",AltitudeStr,sizeof (AltitudeStr) - 1);
   NewAltitude = Map_GetAltitudeFromStr (AltitudeStr);

   /***** Update database changing old altitude by new altitude *****/
   CtrCfg_UpdateCtrCoordinateDB (Gbl.Hierarchy.Ctr.CtrCod,"Altitude",NewAltitude);
   Gbl.Hierarchy.Ctr.Coord.Altitude = NewAltitude;

   /***** Show the form again *****/
   CtrCfg_ShowConfiguration ();
  }

/*****************************************************************************/
/******** Update database changing old coordinate by new coordinate **********/
/*****************************************************************************/

static void CtrCfg_UpdateCtrCoordinateDB (long CtrCod,
					  const char *CoordField,double NewCoord)
  {
   /***** Update database changing old coordinate by new coordinate *****/
   Str_SetDecimalPointToUS ();		// To write the decimal point as a dot
   DB_QueryUPDATE ("can not update a coordinate of a centre",
		   "UPDATE centres SET %s='%.15lg' WHERE CtrCod=%ld",
	           CoordField,NewCoord,CtrCod);
   Str_SetDecimalPointToLocal ();	// Return to local system
  }

/*****************************************************************************/
/************************* Change the URL of a centre ************************/
/*****************************************************************************/

void CtrCfg_ChangeCtrWWW (void)
  {
   extern const char *Txt_The_new_web_address_is_X;
   char NewWWW[Cns_MAX_BYTES_WWW + 1];

   /***** Get parameters from form *****/
   /* Get the new WWW for the centre */
   Par_GetParToText ("WWW",NewWWW,Cns_MAX_BYTES_WWW);

   /***** Check if new WWW is empty *****/
   if (NewWWW[0])
     {
      /***** Update database changing old WWW by new WWW *****/
      Ctr_UpdateCtrWWWDB (Gbl.Hierarchy.Ctr.CtrCod,NewWWW);
      Str_Copy (Gbl.Hierarchy.Ctr.WWW,NewWWW,sizeof (Gbl.Hierarchy.Ctr.WWW) - 1);

      /***** Write message to show the change made *****/
      Ale_ShowAlert (Ale_SUCCESS,Txt_The_new_web_address_is_X,
		     NewWWW);
     }
   else
      Ale_CreateAlertYouCanNotLeaveFieldEmpty ();

   /***** Show the form again *****/
   CtrCfg_ShowConfiguration ();
  }

/*****************************************************************************/
/** Show message of success after changing a centre in centre configuration **/
/*****************************************************************************/

void CtrCfg_ContEditAfterChgCtr (void)
  {
   /***** Write error/success message *****/
   Ale_ShowAlerts (NULL);

   /***** Show the form again *****/
   CtrCfg_ShowConfiguration ();
  }
