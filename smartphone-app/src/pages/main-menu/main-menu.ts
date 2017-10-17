import { Component } from '@angular/core';
import { NavController, ViewController } from 'ionic-angular';
import { TranslateService } from '@ngx-translate/core';
import { GlobalVars } from '../../providers/global-vars';
import { Feedback } from '../../providers/feedback';
import { About } from "../about/about";
import { Introduction } from "../introduction/introduction";

/**
* The Menu
* displayed on HomePage
*/
@Component({
	selector: 'page-main-menu',
	template: `
	<ion-list no-lines>
		<ion-item>
			<ion-label>{{'menu_WIFISYNCONLY' | translate}}</ion-label>
			<ion-checkbox item-right [(ngModel)]="wifiSync" (ionChange)="updateWifiSync()"></ion-checkbox>
		</ion-item>
		<button ion-item *ngIf="feedbackEnabled" (click)="showFeedback()">{{'menu_SENDFEEDBACK' | translate}}</button>
		<button ion-item (click)="showAbout()">{{'menu_ABOUT' | translate}}</button>
		<button ion-item *ngIf="debug" (click)="resetFirstRun()">resetFirstRun</button>
		<button ion-item *ngIf="debug" (click)="switchLang()">{{'menu_switchLang' | translate}}</button>
	</ion-list>
	`
})
export class MainMenu {
	/*
	* should app only sync when connected to wifi?
	*/
	wifiSync: boolean;
	feedbackEnabled: boolean;
	debug: boolean = false;

	constructor(
		public navCtrl: NavController,
		public translate: TranslateService,
		public viewCtrl: ViewController,
		private globalVars: GlobalVars,
		private feedback: Feedback
	) {}

	/**
	* Called with menu creation, directly after constructor.
	* gets needed globalvars from storage and save to variables.
	*/
	ionViewWillEnter(){
		this.globalVars.getSyncOnWifiOnly().then((val:boolean) => {this.wifiSync = val;});
		this.feedback.mailAvailable().then((val:boolean) => {this.feedbackEnabled = val;});
		this.globalVars.getDebug().then((val:boolean) => {this.debug = val;});
	}

	/*
	* persistently safe wifiSync
	*/
	updateWifiSync(){
		this.globalVars.setSyncOnWifiOnly(this.wifiSync);
	}

	/*
	* resets firstRun flag and shows introductional slides
	*/
	resetFirstRun(){
		this.globalVars.setFirstRun(true);
		this.navCtrl.push(Introduction);
	}

	showFeedback(){
		this.feedback.openMail();
	}

	showAbout() {
		this.navCtrl.push(About, {});
		this.close();
	}

	close(){
		this.viewCtrl.dismiss();
	}

	/**
	* Sets new language while cycling through all available languages.
	*/
	switchLang(){
		var langs: string[] = this.translate.getLangs();
		var currentLang: number = langs.indexOf(this.translate.currentLang);
		console.log(this.translate.currentLang + " --> " +langs[(currentLang+1) % langs.length] + " of "+ langs);
		this.translate.use(langs[(currentLang+1) % langs.length]);
	}
}
