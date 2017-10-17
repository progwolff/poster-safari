import { Injectable } from '@angular/core';
import { Platform, App } from 'ionic-angular';
import { Network } from '@ionic-native/network';
import { File } from '@ionic-native/file';
import { BackgroundMode } from '@ionic-native/background-mode';
import 'rxjs/add/operator/map';
import PouchDB from 'pouchdb';

import { EventDTO, PostrEvent } from "../models/eventDTO";
import { GlobalVars } from './global-vars';
import { Imager } from './imager';

import { Util } from './util';

/**
* Provides dataacess to persistent and remote objects.
* Takes care of syncing and persistent storage
*/
@Injectable()
export class DAO {

	/**
	* All posters
	*/
	posters: Array<EventDTO>;
	posterDB: any;
	posterRemote: string;
	posterSync: any;
	events: Array<PostrEvent>;
	eventDB: any;
	eventSync: any;
	eventRemote: any;
	eventRemoteDB: any;
	localNotifications: any; //don't use the ionic plugin (less features), load the cordova plugin directly instead
	backgroundTasks: number;
	/**
	* posters loaded?
	* Array because it will not be destroyed and thus can be referenced from outside.
	* Value is in first element.
	*/
	public loaded:Array<boolean>;

	/**
	* Loads all data from storage to variables.
	* inits notifications, network changes and sync.
	*/
	constructor(
		private network: Network,
		private globalVars: GlobalVars,
		private file: File,
		private imager: Imager,
		private platform: Platform,
		private app: App,
		private backgroundMode: BackgroundMode
	) {
		this.posters = [];
		this.events = [];
		this.loaded = [false];
		this.backgroundTasks = 0;

		platform.ready().then(() => {
			this.localNotifications = window['cordova'].plugins.notification.local;
		});

		this.connectDB();

		network.onchange().subscribe(() => {
			// Wait before getting connection type.
			setTimeout(() => {
				this.sync();
			}, 3000);
		});
	}

	public getPosters():Promise<Array<EventDTO>>{
		return Promise.resolve(this.posters);
	}

	public getPoster(id: string):EventDTO{
		return this.posters.find(function (e) { return e._id == id });
	}

	private getEvent(id: string):PostrEvent{
		return  this.events.find(function (e) { return e._id == id });
	}

	/**
	* get poster by event
	* @param eventID id of event
	* @return posters that have given event
	*/
	private getPosterWithEvent(eventID:string):Array<EventDTO>{
		return this.posters.filter(function (p){ return p.event == eventID})
	}

	private getPosterIndex(id: string):number{
		return this.posters.findIndex(function (e) { return e._id == id });
	}

	private getEventIndex(id: string):number{
		return  this.events.findIndex(function (e) { return e._id == id });
	}

	/**
	* Create and save a new poster.
	* @param imageURI uri to new posterimage
	*/
	public createPoster(imageURI: string):void{
		this.startBackgroundTask();
		// Schedule a single notification to signal event creation
		this.createProgressNotification(0).then((notificationId) => {

			//create a empty document to let the user know we are doing something
			var eventId:string = Util.uuid();
			var doc:EventDTO = new EventDTO(eventId);
			this.posters.unshift(doc);
			//add images
			this.imager.getImages(imageURI).then((images:Array<string>) => {
				doc.setImage(images[0]);
				doc.setImageThumb(images[1]);
				this.posterDB.post(doc).then(()=>{
					this.sync();
				}).catch((error) => {
					console.log(error);
					this.backgroundTaskFinished();
					this.removeNotification(0); // clear progress notification
					this.createNotification('Failed creating images: '+error);
				});
			}).catch((error) => {
				console.log(error);
				this.backgroundTaskFinished();
				this.removeNotification(0); // clear progress notification
				this.createNotification('Failed uploading poster: '+error);
			});//end getImages
		});//end createProgressNotification
	}

	/**
	* update userEvent of local poster on remote db.
	* @param localId posterId of poster with changed userEvent
	*/
	public updatePoster(localId:string):void{
		this.posterDB.get(localId).then((doc) => {
			var poster:EventDTO = this.getPoster(localId);
			doc.userEvent = poster.userEvent;

			for(var attachment in doc["_attachments"]){
				delete doc["_attachments"][attachment]["revpos"];
			}

			this.posterDB.put(JSON.parse(JSON.stringify(doc)))
			//Sync() not neccessary: no new doc.
		}).catch((err) => {
			console.log("updatePoster("+localId+") error: "+err);
		});
	}

	/**
	* Delete Poster by setting '_deleted'-flag.
	* see https://pouchdb.com/api.html#filtered-replication for explenation
	* @param id posterId of to be deleted Poster
	*/
	public deletePoster(id: string):void{
		//Dont delete doc! Set _deleted flag instead
		this.posterDB.get(id).then((doc) => {
			doc._deleted = true;
			this.posterDB.put(doc).then((response) => {
				this.sync();
			}).catch((err) => {
				console.log(err);
			});
		}).catch((err) => {
			console.log(err);
		});
		this.posters.splice(this.getPosterIndex(id), 1);
	}

	/*
	* connect to DB and save data into local variables.
	* different databases are used for debug and release builds.
	*/
	private connectDB(){
		var couchDB:string = 'http://db.postersafari.info:5984/';
		//PouchDB.debug.enable('*');
		PouchDB.debug.disable();
		this.posterDB = new PouchDB('poster');
		this.eventDB = new PouchDB('event');

		this.globalVars.getDebug().then((isDebug)=>{
			if(isDebug){
				this.posterRemote = couchDB + 'debug_poster';
				this.eventRemote = couchDB + 'debug_event';
			}else{
				this.posterRemote = couchDB + 'poster';
				this.eventRemote = couchDB + 'event';
			}
			this.eventRemoteDB = new PouchDB(this.eventRemote);
			this.initData();
			this.sync();
		})
	}

	/*
	* copy posters from local database to variables.
	* changes in DB will be reflected in local variables.
	*/
	private initData():void{
		//create dummy docs with loading animation
		this.posterDB.info().then((info) => {
			for (var i = 0; i < info.doc_count; i++){
				this.posters.push(new EventDTO);
			}

			//copy data from storage to internalVariable
			this.posterDB.allDocs({
				include_docs: true,
				attachments: true
			}).then((result) => {
				result.rows.map((row, index) => {
					Object.assign(this.posters[index], row.doc);
				});
				Util.sortBy(this.posters, {prop: "created", desc: true});
				this.loaded[0] = true;
				this.eventDB.allDocs({include_docs:true}).then((result)=>{
					result.rows.map((row)=>{
						this.events.push(row.doc);
					})
					for(var i = 0; i < this.posters.length; i++){
						this.assignEvent(this.posters[i]);
					}
				})
			}).catch((err) => {console.log(err);});

			//Listen for changes add reflect them in data
			this.posterDB.changes({live: true, since: 'now', include_docs: true, attachments: true}).on('change', (change) => {
				console.log("posterDB.changed: "+change.id)
				let oldPoster:EventDTO = this.getPoster(change.id);
				if(oldPoster != null && change.deleted != true){
					Object.assign(oldPoster, change.doc);
					//this.posters[changedIndex] = p;
					if(oldPoster.event){
						this.assignEvent(oldPoster);
					}
				};
			}).on('error', (err) => {
				console.log(err);
			});

			this.eventDB.changes({live: true, since: 'now', include_docs: true}).on('change',(change) =>{
				console.log("eventDB changed: "+change.id)
				let i = this.getEventIndex(change.id);
				if(i != -1 && change.deleted != true){
					//save in array
					this.events[i] = change.doc;
					//update in every Poster
					this.getPosterWithEvent(this.events[i]._id).forEach((p)=>this.assignEvent(p))
				}
			}).on('error', (err) => {
				console.log(err);
			});
		}).catch((err) => {console.log(err);});
	}

	/**
	* resolve referenced event to serverEvent.
	* Checks local eventDB for referenced event and saves it to field 'serverEvent'.
	* This is done for easy access to values of associated event.
	* @param poster poster without correct serverEvent. Will be overritten by fresh serverEvent
	*/
	private assignEvent(poster:EventDTO){
		if(poster.event && poster.event.length >0){
			var e:PostrEvent = this.getEvent(poster.event);
			if(e == null){
				this.eventRemoteDB.get(poster.event).then((doc)=>{
					poster.serverEvent = Object.assign(new PostrEvent,doc);
					this.events.push(poster.serverEvent);
					this.createEventSync(this.getEventIDs().then((ids)=>{ids.push(doc._id);return ids}));
				}).catch((err) => {console.log(err);});
			}else{
				poster.serverEvent = e;
			}
		}
	}

	/**
	* Handles sync with remote database.
	* syncs only when settings match (currently checks wifi-status + wifisetting)
	* defaults to all documents in local databases are synced
	* @param posterIDs ids of posters that should be synced
	* @param eventIDs ids of events that should be synced
	*/
	private sync(posterIDs?: Promise<Array<string>>,eventIDs?: Promise<Array<string>>):void{
		//get permission status
		this.globalVars.getSyncOnWifiOnly().then((wifiOnly)=>{
			//allowed to sync?
			if(!wifiOnly || (wifiOnly && this.network.type === 'wifi')){
				this.createPosterSync(posterIDs);
				this.createEventSync(eventIDs);
			}//end if wifionly
		})
	}

	/**
	* create sync job posterDB.
	* defaults to all documents in local databases are synced
	* @param posterIDs ids of posters that should be synced
	*/
	private createPosterSync(posterIDs?: Promise<Array<string>>):void{
		//check if special eventIDs are given, default to all local
		if(posterIDs == null){
			posterIDs = this.getPosterIDs();
		}
		posterIDs.then((eIDs)=>{
			//cancel old sync
			if(this.posterSync != null){
				this.posterSync.cancel();
				this.backgroundTaskFinished();//wrong? will not be resumed when with new sync
			}
			//do the sync
			let options = {
				live: true,
				retry: true,
				continuous: true,
				auth:{username:"appUser", password:"Kzeo&d4NTYYZWuc8Ree*"},
				doc_ids:eIDs
			};
			this.posterSync = this.posterDB.sync(this.posterRemote, options);
			this.posterSync.on('change', (info) => {
				console.log('posterSync.onChange');
				if(info.direction != "pull" && info.change !== undefined) {
					this.posterDB.info().then((dbinfo) => {
						if(info.change.last_seq>=dbinfo.update_seq) {
							this.removeNotification(0);
							this.globalVars.getDebug().then((debug)=>{
								if(debug){
									this.createNotification('Upload done');
								}
							});
							this.backgroundTaskFinished();
						}
					});//end posterDB.info
				}
			}).on('denied', function (err) {
				console.log('posterSync: denied! '+err);
			}).on('complete', function (info) {
				console.log('posterSync: complete.');
			}).on('error', function (err) {
				console.log('posterSync: error! '+err);
			});
		});
	}

	/**
	* create sync job eventDB.
	* defaults to all documents in local databases are synced
	* @param eventIDs ids of events that should be synced
	*/
	private createEventSync(eventIDs?: Promise<Array<string>>):void{
		//check if special eventIDs are given, default to all local
		if(eventIDs == null){
			eventIDs = this.getEventIDs();
		}
		eventIDs.then((pIDs)=>{
			//cancel old sync
			if(this.eventSync != null){
				this.eventSync.cancel();
			}
			//do the sync
			let options = {
				live: true,
				retry: true,
				continuous: true,
				auth:{username:"appUser", password:"Kzeo&d4NTYYZWuc8Ree*"},
				doc_ids:pIDs
			};
			console.log("eventSync with these IDs: " + pIDs.toString())
			this.eventSync = this.eventDB.sync(this.eventRemote, options);
		});
	}

	public getPosterIDs():Promise<Array<string>>{
		return this.getIDs(this.posterDB);
	}

	public getEventIDs():Promise<Array<string>>{
		return this.getIDs(this.eventDB);
	}

	private getIDs(db):Promise<Array<string>>{
		return db.allDocs().then((result) => {
			return result.rows.map((row) => { return row.id});
		});
	}

	/**
	* Create notification with progress
	* @param id id of notification
	* @param text displayed text
	* @return id of creted notification
	*/
	private createProgressNotification(id: number = -1, text: string = 'Uploading a poster to the server'):Promise<number> {
		return new Promise((resolve, reject) => {
			this.localNotifications.getAllIds((ids) => {
				if(id == -1) {
					while(ids.indexOf(id) >= 0)
					++id;
				}else{
					id = 0;
				}
				this.localNotifications.schedule({
					id: id,
					text: text,
					progress: true,
					maxProgress: 100,
					currentProgress: 10,
					autoClear: false,
					sound: "none",
					ongoing: true
				});
				resolve(id);
			});
		});
	}

	/**
	* Create notification
	* @param id id of notification
	* @param text displayed text
	* @return id of creted notification
	*/
	private createNotification(text: string = 'Uploading a poster to the server', id: number = -1):Promise<number> {
		return new Promise((resolve, reject) => {
			this.localNotifications.getAllIds((ids) => {
				if(id == -1) {
					while(ids.indexOf(id) >= 0)
					++id;
				}else{
					id = 0;
				}
				this.localNotifications.schedule({
					id: id,
					text: text,
					sound: "none"
				});
				resolve(id);
			});
		});
	}

	/**
	* sets progress in notification
	* @param notificationId id of notification
	* @param progress in percent
	*/
	private updateProgressNotification(notificationId: number = 0, progress: number = 0) {
		this.localNotifications.update({
			id: notificationId,
			currentProgress: progress
		});
	}

	private removeNotification(notificationId: number = 0) {
		this.localNotifications.cancel(notificationId);
	}

	/*
	* enables backgroundMode such that app is not closed by OS.
	* can be called multible times forEach task in background
	*/
	private startBackgroundTask():void {
		this.backgroundTasks++;
		this.backgroundMode.setDefaults({
			title: "Poster Safari",
			text: "Running background tasks",
			resume: true,
			hidden: true
		});
		this.backgroundMode.enable();
		//this.backgroundMode.on('activate').subscribe(() => {
		//this.backgroundMode.disableWebViewOptimizations();
		//});
		//this.backgroundMode.overrideBackButton();
		//console.log("background mode enabled");
	}

	/**
	* indicates a backgroundTask finished and if none are running anymore disables backgroundMode.
	*/
	private backgroundTaskFinished():void {
		this.backgroundTasks--;
		if(this.backgroundTasks < 0)
		this.backgroundTasks = 0;
		if(this.backgroundTasks == 0){
			this.backgroundMode.disable();
		}
	}
}
