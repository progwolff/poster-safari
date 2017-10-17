import { Component } from '@angular/core';
import { NavController } from 'ionic-angular';
import { HomePage } from "../home/home";
import { GlobalVars } from "../../providers/global-vars";

/**
* Shows tutorial slides
*/
@Component({
	templateUrl: 'introduction.html'
})
export class Introduction {
	constructor(private navCtrl: NavController, private globalVars: GlobalVars) {}

	/**
	* skip slides and navigate to HomePage
	*/
	skip(){
		this.navCtrl.setRoot(HomePage);
		this.globalVars.setFirstRun(false);
	}

	/**
	* end slides and open camera
	*/
	end(){
		this.navCtrl.setRoot(HomePage,{ showCamera: true });
		this.globalVars.setFirstRun(false);
	}
}
