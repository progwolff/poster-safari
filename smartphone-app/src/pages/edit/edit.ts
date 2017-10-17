import { Component } from '@angular/core';
//import { FormGroup, FormControl, FormArray, FormBuilder, Validators } from '@angular/forms';
import { NavParams, ModalController, ViewController } from 'ionic-angular';
import { DAO } from "../../providers/dao";
import { TranslateService } from '@ngx-translate/core';
import { EventDTO, PostrEvent, Time, Ticket } from "../../models/eventDTO";
import { GlobalVars } from "../../providers/global-vars";
import { ImageView } from "../imageview/imageview";

/**
* Show edit page. User can change data about event here
*/
@Component({
	selector: 'page-details',
	templateUrl: 'edit.html',
})
export class Edit {
	/**
	* poster that is edited
	*/
	poster: EventDTO;
	/**
	* temoprary object that will be edited
	*/
	editEvent: PostrEvent;

	constructor(
		public navParams: NavParams,
		public modalCtrl: ModalController,
		public viewCtrl:ViewController,
		private translate: TranslateService,
		private globalVars: GlobalVars,
		private dao: DAO) {
		this.poster = navParams.get('poster');

		if(this.poster.serverEvent){
			//may seem a bit hacky. parse(stringify()) to get fast deepCopy. object.assign takes DatenIso-strings so JSON.stringify does not break dates
			this.editEvent = Object.assign(new PostrEvent(),JSON.parse(JSON.stringify(this.poster.serverEvent)),JSON.parse(JSON.stringify(this.poster.userEvent)));
		}else{
			this.editEvent = this.poster.userEvent;
		}
		console.log(this.editEvent);
	}


	/**
	 * WARNING: not robust, not universal!
	 * copy only the different values between original and edited to diff
	 * Arrays are copied complete, because the mapping will work nomore if not whole array is in diff
	 * @param edited source of values that will be copied to 'diff'
	 * @param original source of values that will not be copied when in 'edited'
	 * @param diff new object. Here will all attributes be that are differen in 'edited' than in 'source'
	 */
	private copyDiffs(edited, original, diff){
		for(var property in edited){
			if (typeof edited[property] == "object" && !Array.isArray(edited[property])) {
				diff[property] = {};
				this.copyDiffs(edited[property],original[property],diff[property]);
			}else if((Array.isArray(edited[property]))){
				if(JSON.stringify(edited[property]) !== JSON.stringify(original[property])){
					diff[property] = edited[property];
				}
			}else if(edited[property] !== original[property]){
				diff[property] = edited[property];
			}
		}
	}

	/**
	* save the edited values to the poster and save it.
	*/
	processEdits(){
		if(this.poster.serverEvent){
			var newUserEvent = new PostrEvent();
			this.copyDiffs(this.editEvent, this.poster.serverEvent, newUserEvent)
			this.poster.userEvent = newUserEvent;
		}
		this.dao.updatePoster(this.poster._id);
		this.dismiss(true);
		console.log("Edit done");
	}

	getTimeFormat(time:Time):string{
		if(time.allDay){
			return "DD.MM.YYYY"
		}else{
			return "DD.MM.YY HH:mm"
		}
	}

	getPickerFormat(time:Time):string{
		if(time.allDay){
			return "DD.MM.YYYY"
		}else{
			return "DD.MM.YYYY, HH:mm"
		}
	}

	/**
	* add new date to event
	*/
	addDate(){
		if(!this.editEvent.times){
			this.editEvent.times = new Array<Time>();
		}
		this.editEvent.times.push(new Time);
	}

	/**
	* remove date from event
	* @param i id of date in array
	*/
	removeDate(i:number){
		if(this.editEvent.times && this.editEvent.times.length >i){
			this.editEvent.times.splice(i,1);
		}
	}

	/**
	* add new ticket to event
	*/
	addTicket(){
		if(!this.editEvent.tickets){
			this.editEvent.tickets = new Array<Ticket>();
		}
		this.editEvent.tickets.push(new Ticket);
	}

	/**
	* remove ticket from event
	* @param i id of ticket in array
	*/
	removeTicket(i:number){
		if(this.editEvent.tickets && this.editEvent.tickets.length >i){
			this.editEvent.tickets.splice(i,1);
		}
	}

	/**
	* add new artist to event
	*/
	addArtist(){
		if(!this.editEvent.artists){
			this.editEvent.artists = new Array<string>();
		}
		this.editEvent.artists.push("");
	}

	/**
	* remove artist from event
	* @param i id of artist in array
	*/
	removeArtist(i:number){
		if(this.editEvent.artists && this.editEvent.artists.length >i){
			this.editEvent.artists.splice(i,1);
		}
	}

	/**
	* add new organizer to event
	*/
	addOrganizer(){
		if(!this.editEvent.organizers){
			this.editEvent.organizers = new Array<string>();
		}
		this.editEvent.organizers.push("");
	}

	/**
	* remove organizer from event
	* @param i id of organizer in array
	*/
	removeOrganizer(i:number){
		if(this.editEvent.organizers && this.editEvent.organizers.length >i){
			this.editEvent.organizers.splice(i,1);
		}
	}

	/**
	* show image of poster in fullscreen
	*/
	private viewImage() {
		let imageModal = this.modalCtrl.create(ImageView, { image: this.poster.getFullSizePicture() });
		imageModal.present();
	}

	/**
	* close editpane
	* @param includePoster whether the edited poster should be returned. If true edited version will be saved.
	*/
	dismiss(includePoster:boolean) {
		if(includePoster){
			this.viewCtrl.dismiss(this.poster);
		}else{
			this.viewCtrl.dismiss();
		}

 }
}
