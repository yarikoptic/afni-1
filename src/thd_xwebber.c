#include "mrilib.h"
#include "xutil.h"
#include "XmHTML/XmHTML.h"
#include "debugtrace.h"    /* 12 Mar 2001 */

XmImageConfig *_xmimage_cfg = NULL ;

typedef struct {
   Widget wshell , wtop , wactar , wframe , whtml ;
   void_func *kill_func ;
   XtPointer  kill_data ;
   int shell_width , shell_height ;
} MCW_htmlwin ;

void MCW_htmlwinkill_CB( Widget , XtPointer , XtPointer ) ;
void MCW_htmlwin_CB    ( Widget , XtPointer , XtPointer ) ;

static MCW_action_item HWIN_act[] = {
 { "Quit" , MCW_htmlwin_CB , NULL , NULL , "Close window" , 0 } ,
} ;

/*----------------------------------------------------------------------------*/

static void anchorCB( Widget widget, XtPointer client_data,
                      XmHTMLAnchorCallbackStruct *cbs      )
{
  cbs->doit = True ; cbs->visited = True ;
}

/*----------------------------------------------------------------------------*/

static char *htmlize( char *msg )
{
   char *mmm=NULL ;

   if( msg == NULL || *msg == '\0'  )
     msg = strdup("<html><body><h1>Dummy</h1><h2>Message</h2></body></html>") ;

   if( strncmp(msg,"<html>",6) == 0 ) return msg ;

   if( strncmp(msg,"file:",5) == 0 ){      /* read file */
     char *qqq=AFNI_suck_file(msg+5) ; char *base,*dnam ; int nbas ;
     if( qqq != NULL )
       mmm = qqq ;
     else
       mmm = strdup("<html><body><h1>Dummy</h1><h2>Message</h2></body></html>") ;

     base = strcasestr(mmm,"<base href=>") ;
     if( base != NULL ){
       dnam = strdup(msg+5) ; qqq  = THD_trailname(dnam,0) ;
       if( qqq != NULL && qqq != dnam ){
         *qqq = '\0' ;
         nbas = 11 + (base-mmm) ;
         qqq  = (char *)malloc(strlen(mmm)+strlen(dnam)+64) ;
         memcpy( qqq , mmm , nbas ) ; qqq[nbas] = '\0' ;
         strcat( qqq , "\"" ) ; strcat( qqq , dnam ) ; strcat( qqq , "\">" ) ;
         strcat( qqq , base + 12 ) ;
         free(mmm) ; mmm = qqq ;
       }
       free(dnam) ;
     }

   } else {                                /* add HTML stuff */
     mmm = (char *)malloc(sizeof(char)*(strlen(msg)+64)) ;
     strcpy(mmm,"<html><body>\n") ;
     strcat(mmm,msg) ;
     strcat(mmm,"\n</body></html>") ;
   }

   return mmm ;
}

/*----------------------------------------------------------------------------*/

MCW_htmlwin * new_MCW_htmlwin( Widget wpar , char *msg ,
                               void_func *kill_func , XtPointer kill_data )

{
   int wx,hy,xx,yy , xp,yp , scr_width,scr_height , xr,yr , xpr,ypr , ii,nact ;
   int swid , shi ;
   Position xroot , yroot ;
   Screen *scr ;
   Arg wa[64] ; int na ; Widget ws ;
   char *wtype = "help" ;
   MCW_htmlwin *hw ;
   char *mymsg ;

ENTRY("new_MCW_htmlwin") ;

   /*-- sanity check --*/

   if( wpar == NULL || !XtIsRealized(wpar) || msg == NULL || *msg == '\0' )
     EXRETURN ;

   /* set position based on parent and screen geometry */

   MCW_widget_geom( wpar , &wx,&hy,&xx,&yy ) ;     /* geometry of parent */
   XtTranslateCoords( wpar, 0,0, &xroot,&yroot ) ; /* root coords */
   xr = (int) xroot ; yr = (int) yroot ;

   scr        = XtScreen(wpar) ;
   scr_width  = WidthOfScreen(scr) ;
   scr_height = HeightOfScreen(scr) ;

   xp = xx+8 ;  xpr = xr+8 ;
        if( xpr+50 > scr_width ){ xp -= 100 ; xpr -= 100 ; } /* too right */
   else if( xpr+10 < 0 )        { xpr = xp = 1 ; }           /* too left  */

   yp = yy+hy+8 ;  ypr = yr+hy+8 ;
        if( ypr+50 > scr_height ){ yp = yy-8 ; ypr = yr-100 ;} /* too down */
   else if( ypr+10 < 0 )         { ypr = yp = 1 ;            } /* too up   */

   /* create a popup shell */

   hw = myXtNew(MCW_htmlwin) ;
   hw->kill_func = kill_func ;
   hw->kill_data = kill_data ;

   hw->wshell = XtVaCreatePopupShell(
                 wtype , xmDialogShellWidgetClass , wpar ,
                    XmNx , xpr ,
                    XmNy , ypr ,
                    XmNborderWidth , 0 ,
                    XmNborderColor , 0 ,
                    XmNinitialResourcesPersistent , False ,
                 NULL ) ;

   XmAddWMProtocolCallback(
        hw->wshell ,
        XmInternAtom( XtDisplay(hw->wshell) , "WM_DELETE_WINDOW" , False ) ,
        MCW_htmlwinkill_CB , (XtPointer)hw ) ;

   /* create a form to hold everything else */

   hw->wtop = XtVaCreateWidget(
                wtype , xmFormWidgetClass , hw->wshell ,
                  XmNborderWidth , 0 ,
                  XmNborderColor , 0 ,
                  XmNtraversalOn , True  ,
                  XmNinitialResourcesPersistent , False ,
                NULL ) ;

   /* create action area */

   nact = 1 ;
   for( ii=0 ; ii < nact ; ii++ ){
     HWIN_act[ii].data     = (XtPointer)hw ;
     HWIN_act[ii].make_red = 0 ;
   }
   HWIN_act[nact-1].make_red = 1 ;

   hw->wactar = MCW_action_area( hw->wtop , HWIN_act , nact ) ;

   XtVaSetValues( hw->wactar ,
                     XmNleftAttachment , XmATTACH_FORM ,
                     XmNrightAttachment, XmATTACH_FORM ,
                     XmNtopAttachment  , XmATTACH_FORM ,
                     XmNtopOffset      , 4 ,
                  NULL ) ;

   /* frame to hold HTML widget */

   hw->wframe = XtVaCreateManagedWidget(
                  wtype , xmFrameWidgetClass, hw->wtop ,
                  XmNtopAttachment   , XmATTACH_WIDGET ,
                  XmNtopWidget       , hw->wactar ,
                  XmNtopOffset       , 4 ,
                  XmNleftAttachment  , XmATTACH_FORM,
                  XmNleftOffset      , 1 ,
                  XmNbottomAttachment, XmATTACH_FORM,
                  XmNbottomOffset    , 1 ,
                  XmNrightAttachment , XmATTACH_FORM,
                  XmNrightOffset     , 1,
                  XmNshadowType      , XmSHADOW_IN,
                  XmNshadowThickness , 4 ,
                NULL ) ;

   /* create HTML area */

   swid = WidthOfScreen(XtScreen(wpar))  - 222 ; if( swid > 777 ) swid = 777 ;
   shi  = HeightOfScreen(XtScreen(wpar)) - 222 ; if( shi  > 888 ) shi  = 888 ;

   mymsg = htmlize(msg) ;

   hw->whtml = XtVaCreateManagedWidget(
                  wtype , xmHTMLWidgetClass , hw->wframe ,
                  XmNmarginWidth       , 4 ,
                  XmNmarginHeight      , 4 ,
                  XmNwidth             , swid ,
                  XmNheight            , shi ,
                  XmNvalue             , mymsg,
                  XmNfontFamily        , "adobe-helvetica-normal-*" ,
                  XmNfontFamilyFixed   , "adobe-courier-normal-*" ,
                  XmNfontSizeFixedList , "14,10" ,
                  XmNanchorButtons     , False ,
                NULL ) ;
   XtAddCallback( hw->whtml, XmNactivateCallback, (XtCallbackProc)anchorCB, NULL ) ;

   XtManageChild( hw->wtop ) ;

#if 0
   XtVaSetValues( hw->wshell , XmNwidth,swid , XmNheight,shi , NULL ) ;
#endif

   XtPopup( hw->wshell , XtGrabNone ) ; RWC_sleep(16) ;

   RWC_visibilize_widget( hw->wshell ) ;

   RWC_xineramize( XtDisplay(hw->wshell) ,
                   xpr,ypr,swid,shi , &xpr,&ypr ) ;

   XtVaSetValues( hw->wshell, XmNx,xpr , XmNy,ypr , NULL ) ;

   hw->shell_width = swid ; hw->shell_height = shi ;

   NORMAL_cursorize( hw->wshell ) ;

#if 0
   XmHTMLTextSetString( hw->whtml , mymsg ) ;
#endif

   if( mymsg != msg ) free(mymsg) ;

   RWC_sleep(66) ;

   XmHTMLRedisplay( hw->whtml ) ;

   RETURN(hw) ;
}

/*-------------------------------------------------------------------------*/

void MCW_htmlwin_alter( MCW_htmlwin *hw , char *mmm )
{
   int swid , shi ;
   char *msg ;

ENTRY("MCW_htmlwin_alter") ;

   if( hw == NULL || mmm == NULL || *mmm == '\0' ) EXRETURN ;

   msg = htmlize(mmm) ;

   XmHTMLTextSetString( hw->whtml , msg ) ;

   if( msg != mmm ) free(msg) ;

   EXRETURN ;
}

/*-------------------------------------------------------------------------*/

void MCW_htmlwin_CB( Widget w , XtPointer client_data , XtPointer call_data )
{
   MCW_htmlwin *hw = (MCW_htmlwin *)client_data ;
   char *wname     = XtName(w) ;

   if( client_data == NULL ) return ;

   if( strcmp(wname,"Quit") == 0 ){
      if( hw->kill_func != NULL )
        AFNI_CALL_VOID_1ARG( hw->kill_func , XtPointer , hw->kill_data ) ;
      XtDestroyWidget( hw->wshell ) ;
      myXtFree( hw ) ;
      return ;
   }

   return ;
}

/*-------------------------------------------------------------------------*/

void MCW_htmlwinkill_CB( Widget w, XtPointer client_data, XtPointer call_data )
{
   MCW_htmlwin *hw = (MCW_htmlwin *) client_data ;

   if( hw->kill_func != NULL )
     AFNI_CALL_VOID_1ARG( hw->kill_func , XtPointer , hw->kill_data ) ;
   XtDestroyWidget( hw->wshell ) ;
   myXtFree( hw ) ;
   return ;
}