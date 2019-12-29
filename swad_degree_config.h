// swad_degree_config.h: configuration of current degree

#ifndef _SWAD_DEG_CFG
#define _SWAD_DEG_CFG
/*
    SWAD (Shared Workspace At a Distance in Spanish),
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

/*****************************************************************************/
/***************************** Public constants ******************************/
/*****************************************************************************/

/*****************************************************************************/
/******************************* Public types ********************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Public prototypes *****************************/
/*****************************************************************************/

void DegCfg_ShowConfiguration (void);
void DegCfg_PrintConfiguration (void);

void DegCfg_ChangeDegCtrInConfig (void);
void DegCfg_RenameDegreeShortInConfig (void);
void DegCfg_RenameDegreeFullInConfig (void);
void DegCfg_ChangeDegWWWInConfig (void);
void DegCfg_ContEditAfterChgDegInConfig (void);

#endif
