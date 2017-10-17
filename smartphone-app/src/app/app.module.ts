import { BrowserModule } from '@angular/platform-browser';
import { ErrorHandler, NgModule } from '@angular/core';
import { HttpModule, Http } from '@angular/http';
import { IonicApp, IonicErrorHandler, IonicModule } from 'ionic-angular';
import { IonicStorageModule } from '@ionic/storage';
import { StatusBar } from '@ionic-native/status-bar';
import { SplashScreen } from '@ionic-native/splash-screen';
import { Camera } from "@ionic-native/camera";
import { Calendar } from '@ionic-native/calendar';
import { Network } from '@ionic-native/network';
import { SocialSharing } from '@ionic-native/social-sharing';
import { EmailComposer } from '@ionic-native/email-composer';
import { File } from '@ionic-native/file';
import { IsDebug } from '@ionic-native/is-debug';
import { TranslateModule, TranslateLoader } from '@ngx-translate/core';
import { TranslateHttpLoader } from '@ngx-translate/http-loader';

//Pages
import { MyApp } from './app.component';
import { HomePage } from '../pages/home/home';
import { Details } from "../pages/details/details";
import { Edit } from "../pages/edit/edit";
import { MainMenu } from "../pages/main-menu/main-menu";
import { About } from "../pages/about/about";
import { Introduction } from "../pages/introduction/introduction";
import { ImageView } from "../pages/imageview/imageview";

//Providers
import { DAO } from "../providers/dao";
import { GlobalVars } from "../providers/global-vars";
import { Feedback } from "../providers/feedback";
import { Imager } from "../providers/imager";
import { BackgroundMode } from '@ionic-native/background-mode';

@NgModule({
	declarations: [
		MyApp,
		HomePage,
		Details,
		Edit,
		About,
		MainMenu,
		Introduction,
		ImageView
	],
	imports: [
		BrowserModule,
		IonicModule.forRoot(MyApp),
		IonicStorageModule.forRoot(),
		HttpModule,
		TranslateModule.forRoot({
			loader: {
				provide: TranslateLoader,
				useFactory: createTranslateLoader,
				deps: [Http]
			}
		})
	],
	bootstrap: [IonicApp],
	entryComponents: [
		MyApp,
		HomePage,
		Details,
		Edit,
		About,
		MainMenu,
		Introduction,
		ImageView
	],
	providers: [
		StatusBar,
		SplashScreen,
		Camera,
		Calendar,
		File,
		Network,
		EmailComposer,
		IsDebug,
		SocialSharing,
		DAO,
		GlobalVars,
		Feedback,
		Imager,
		BackgroundMode,
		{provide: ErrorHandler, useClass: IonicErrorHandler}
	]
})
export class AppModule {}

export function createTranslateLoader(http: Http) {
	return new TranslateHttpLoader(http, './assets/i18n/', '.json');
}
