import { Component } from '@angular/core';
import { Platform, App } from 'ionic-angular';
import { NavParams, ModalController, ToastController } from 'ionic-angular';
import { Calendar } from '@ionic-native/calendar';
import { TranslateService } from '@ngx-translate/core';
import { SocialSharing } from '@ionic-native/social-sharing';
import { DAO } from "../../providers/dao";
import { GlobalVars } from "../../providers/global-vars";
import { EventDTO, PostrEvent} from "../../models/eventDTO";
import { ImageView } from "../imageview/imageview";
import { Edit } from "../edit/edit";

/**
* Shows detailed information about a single event
*/
@Component({
	selector: 'page-details',
	templateUrl: 'details.html',
})
export class Details {
	/**
	* displayed event
	*/
	poster: EventDTO;

	/**
	* temporary joined event of userEvent and serverEvent (userEvent dominant)
	*/
	editEvent: PostrEvent;
	debug: boolean = false;
	modal: any;

	constructor(
		public navParams: NavParams,
		private calendar: Calendar,
		private socialSharing: SocialSharing,
		private golbalVars: GlobalVars,
		private modalCtrl: ModalController,
		private platform: Platform,
		private app: App,
		private translate: TranslateService,
		private toastCtrl: ToastController,
		private dao: DAO
	) {
		platform.registerBackButtonAction(() => {
			let nav = app.getActiveNav();
			let activeView: any = nav.getActive();

			if(activeView != null){
				if(nav.canGoBack()) {
					nav.pop();
				}
				else if(this.modal) {
					this.modal.dismiss();
				}
			}
		}, 100);

		this.poster = navParams.get('poster');
		this.golbalVars.getDebug().then((val:boolean) => {this.debug = val;});

		this.initEditEvent();

		console.log(this.editEvent);
	}

	/**
	* create the editEvent
	* editEvent: temporary joined event of userEvent and serverEvent (userEvent dominant)
	*/
	initEditEvent():void{
		if(this.poster.serverEvent){
			//may seem a bit hacky. parse(stringify()) to get fast deepCopy. object.assign takes DatenIso-strings so JSON.stringify does not break dates
			this.editEvent = Object.assign(new PostrEvent(),JSON.parse(JSON.stringify(this.poster.serverEvent)),JSON.parse(JSON.stringify(this.poster.userEvent)));
		}else{
			this.editEvent = Object.assign(new PostrEvent(),JSON.parse(JSON.stringify(this.poster.userEvent)));
		}
	}

	/**
	* show edit window
	*/
	edit():void{
		let editModal = this.modalCtrl.create(Edit, { poster: this.poster},{cssClass: "page-edit"});
		editModal.onDidDismiss(poster => {
			this.initEditEvent();
		});
		this.modal = editModal;
		editModal.present();
	}

	/**
	* create calendar event and open calendarapp for save
	*/
	exportCalendar():void {
		if(this.editEvent.times && this.editEvent.times[0].start && this.editEvent.times[0].stop){
			this.calendar.createEventInteractively(
				this.editEvent.title,
				this.editEvent.getAddress(),
				(this.editEvent.description?this.editEvent.description:""),
				new Date(this.editEvent.times[0].start),
				new Date(this.editEvent.times[0].stop)
			);
		}
		else if(this.editEvent.times && this.editEvent.times[0].start){
			this.calendar.createEventInteractively(
				this.editEvent.title,
				this.editEvent.getAddress(),
				(this.editEvent.description?this.editEvent.description:""),
				new Date(this.editEvent.times[0].start)
			);
		}
		else {
			this.calendar.createEventInteractively(
				this.editEvent.title,
				this.editEvent.getAddress(),
				(this.editEvent.description?this.editEvent.description:""),
			);
		}
	}

	/**
	* share the event via OS sharing function. description+image
	*/
	share():void {
		this.socialSharing.share(this.editEvent.beautify()+"\n\n\nshared with Poster Safari\n", this.editEvent.title, this.poster.getPicture(), "");
	}

	/**
	* dummy function
	*/
	buy():void {
		console.log("buy");
	}

	/**
	* Open navigation to eventlocation
	*/
	navigate():void {
		// see https://developer.android.com/guide/components/intents-common.html
		window.open("geo://0,0?q="+this.editEvent.latitude+","+this.editEvent.longitude+"("+this.editEvent.getVenueName()+")", "_system");
	}

	/**
	* show event image in fullscreen
	*/
	private viewImage():void {
		console.log("viewImage triggered");
		//console.log("image: "+ this.poster.getFullSizePicture());
		let imageModal = this.modalCtrl.create(ImageView, { image: this.poster.getFullSizePicture() });
		this.modal = imageModal;
		imageModal.present();
	 }
}
