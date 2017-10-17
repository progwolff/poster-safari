import { Component, ViewChild } from '@angular/core';
import { Nav, Platform } from 'ionic-angular';
import { StatusBar } from '@ionic-native/status-bar';
import { SplashScreen } from '@ionic-native/splash-screen';
import { TranslateService } from '@ngx-translate/core';

import { HomePage } from '../pages/home/home';

/**
* The App
*/
@Component({
	templateUrl: 'app.html'
})
export class MyApp {
	@ViewChild(Nav) nav: Nav;

	rootPage: any = HomePage;

	pages: Array<{title: string, component: any}>
	constructor(public platform: Platform, public statusBar: StatusBar, public splashScreen: SplashScreen,public translate: TranslateService) {
		this.initTranslate();
	}

	ionViewDidLoad() {
		this.platform.ready().then(() => {
			// Okay, so the platform is ready and our plugins are available.
			// Here you can do any higher level native things you might need.
			this.splashScreen.hide();
		});
	}

	/**
	* set language, default language and available languages
	*/
	initTranslate() {
		// Set the default language for translation strings, and the current language.
		this.translate.setDefaultLang('en');
		this.translate.addLangs(['en','de']);

		if (this.translate.getBrowserLang() !== undefined) {
			this.translate.use(this.translate.getBrowserLang());
		} else {
			this.translate.use('en'); // Set your language here
		}
	}

	openPage(page) {
		// Reset the content nav to have just this page
		// we wouldn't want the back button to show in this scenario
		this.nav.setRoot(page.component);
	}
}
