import {Injectable} from '@angular/core';
import { Storage } from '@ionic/storage';
import { IsDebug } from '@ionic-native/is-debug';

const wifiSyncKey:string = 'syncOnWifiOnly';
const firstRun:string = 'firstRun';

/**
* Provides persistent global variables
*/
@Injectable()
export class GlobalVars {
	/**
	* true if values were loaded from persistent storage
	*/
	private loaded: boolean;
	private syncOnWifiOnly: boolean;
	private firstRun: boolean;
	private debug: boolean;

	constructor(private storage: Storage, private isDebug: IsDebug) {
		this.loaded = false;
		this.syncOnWifiOnly = true;
		this.firstRun = false;
		this.debug = false;
	}

	/**
	* loads all globalVars from storage and saves them to variables for faster access
	* @return Resolves when loaded
	*/
	private loadFromStorage():Promise<boolean>{
		var proms:Array<Promise<void>> = [];
		return this.storage.ready().then(() => {
			proms.push(this.isDebug.getIsDebug().then((r: boolean) => { this.debug = r; }));
			proms.push(this.storage.get(wifiSyncKey).then((r) => { this.syncOnWifiOnly=r!=0; }));
			proms.push(this.storage.get(firstRun).then((r) => { this.firstRun=r!=0; }));
			return Promise.all(proms).then((r)=>{
				this.loaded = true;
				return true;
			}).catch((error: any) => { console.error(error) });
		});
	}

	setSyncOnWifiOnly(value:boolean) {
		this.syncOnWifiOnly = value;
		this.storage.set(wifiSyncKey, value?1:0)
	}

	getSyncOnWifiOnly():Promise<boolean> {
		if(!this.loaded){
			return this.loadFromStorage().then(() => { return this.syncOnWifiOnly; });
		}else{
			return Promise.resolve(this.syncOnWifiOnly);
		}
	}

	setFirstRun(value:boolean) {
		this.firstRun = value;
		this.storage.set(firstRun, value?1:0);
	}

	getFirstRun():Promise<boolean> {
		if(!this.loaded){
			return this.loadFromStorage().then(() => {return this.firstRun; });
		}else{
			return Promise.resolve(this.firstRun);
		}
	}

	getDebug():Promise<boolean> {
		if(!this.loaded){
			return this.loadFromStorage().then(() => { return this.debug; });
		}else{
			return Promise.resolve(this.debug);
		}
	}
}
