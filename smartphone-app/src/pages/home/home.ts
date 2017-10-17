import { Component } from '@angular/core';
import { NavController, NavParams, PopoverController, ActionSheetController, Platform, App} from 'ionic-angular';
import { Camera, CameraOptions } from '@ionic-native/camera';
import { TranslateService } from '@ngx-translate/core';
import { Introduction } from "../introduction/introduction";
import { BackgroundMode } from '@ionic-native/background-mode';
import { Details } from "../details/details";
import { MainMenu } from "../main-menu/main-menu";
import { DAO } from "../../providers/dao";
import { GlobalVars } from "../../providers/global-vars";
import { EventDTO, PostrEvent } from "../../models/eventDTO";

/**
* Shows overview over all posters.
*/
@Component({
	selector: 'page-home',
	templateUrl: 'home.html'
})
export class HomePage {
	/**
	* all the posters
	*/
	posters: Array<EventDTO>;
	debug: boolean = false;
	loaded: Array<boolean> = [false];
	cameraShown: boolean = false;
	actionSheet: any = null;
	popover: any = null;
	fabButton: any = null;

	constructor(
		public navCtrl: NavController,
		public navParams: NavParams,
		private dao: DAO,
		private globalVars: GlobalVars,
		private camera: Camera,
		private platform: Platform,
		private app: App,
		private backgroundMode: BackgroundMode,
		public popoverCtrl: PopoverController,
		public actionSheetCtrl: ActionSheetController,
		public translate: TranslateService
	) {
	}

	/**
	* init posters, manage backbutton
	*/
	ionViewWillEnter(){
		this.platform.registerBackButtonAction(() => {
			let nav = this.app.getActiveNav();
			let activeView: any = nav.getActive();
			if(activeView != null){
				if(this.actionSheet) {
					this.actionSheet.dismiss();
				} else if(this.popover) {
					this.popover.dismiss();
				} else if(this.fabButton) {
					this.closeFab();
				} else if(!nav.canGoBack()) {
					this.backgroundMode.moveToBackground();
				}
			}
		}, 100);

		this.posters=[];
		this.loaded = this.dao.loaded;

		this.checkShowIntroduction();
		if(this.navParams.get('showCamera')){
			if(!this.cameraShown){
				this.cameraShown = true;
				this.getPicture(false);
			}
		}
		this.dao.getPosters()
		.then((data:Array<EventDTO>) => {this.posters = data});
		this.globalVars.getDebug()
		.then((isDebug: boolean) => {this.debug = true;})
		.catch(console.error);
	}

	showDetails(poster: PostrEvent):void {
		this.navCtrl.push(Details, { poster: poster });
		this.closeFab();
	}

	registerFab(fab):void {
		this.fabButton = fab;
	}

	/**
	* Close fab, called when tabbed in background and fab is open.
	*/
	bdClick():void{
		this.closeFab();
	}

	/**
	* close the FAB button
	*/
	closeFab(){
		if(this.fabButton) {
			this.fabButton.close();
			this.fabButton = null;
		}
	}

	/**
	* open contex Menu
	* @param event the event for that the menu will be opend
	*/
	contextMenu(event):void {
		this.translate.get(['home_EVENT','home_REMOVE','home_DETAILS','home_CANCEL']).subscribe( t => {
			this.actionSheet = this.actionSheetCtrl.create({
				title: event.getEventVar('title'),
				buttons: [
					{
						text: t.home_DETAILS,
						icon: 'information-circle',
						handler: () => {
							this.showDetails(event);
						}
					},{
						text: t.home_REMOVE,
						icon: 'trash',
						role: 'destructive',
						handler: () => {
							this.dao.deletePoster(event._id);
						}
					},{
						text: t.home_CANCEL,
						icon: 'arrow-back',
						role: 'cancel'
					}
				]
			});
			this.actionSheet.onDidDismiss(() => {
				this.actionSheet = null;
			});
			this.actionSheet.present();
		});
	}

	/**
	* create the menu
	* @param event information about where the menu should be created
	*/
	presentPopover(event) {
		this.popover = this.popoverCtrl.create(MainMenu);
		this.popover.onDidDismiss(() => {
			this.popover = null;
		});
		this.popover.present({
			ev: event
		});
		this.closeFab();
	}

	/**
	* checks if the Introduction should be shown and shows it in case.
	*/
	checkShowIntroduction(){
		this.globalVars.getFirstRun().then((firstRun)=>{
			if(firstRun){
				this.navCtrl.setRoot(Introduction);
			}
		})
	}

	/**
	* retrives image from camera or album. this will be saved in storage and thus shown in overview
	* @param fromAlbum whether image should be retrived from album or camera
	*/
	getPicture(fromAlbum: boolean = false){
		const options: CameraOptions = {
			quality: 100,
			destinationType: this.camera.DestinationType.FILE_URI,
			encodingType: this.camera.EncodingType.JPEG,
			mediaType: this.camera.MediaType.PICTURE,
			correctOrientation: false, //Too memory expensive. Rotate on server using the EXIF tag instead.
			sourceType: (fromAlbum ? this.camera.PictureSourceType.PHOTOLIBRARY : this.camera.PictureSourceType.CAMERA)
		}
		this.camera.getPicture(options).then((imageURI)=>{this.dao.createPoster(imageURI)}).catch(console.error);
		this.closeFab();
	}
}
