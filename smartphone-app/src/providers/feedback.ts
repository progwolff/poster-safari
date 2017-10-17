import { Injectable } from '@angular/core';
import { EmailComposer } from '@ionic-native/email-composer';
import { TranslateService } from '@ngx-translate/core';
import { DAO } from './dao';

/**
* Provides Feedback ability, currently sends prefilled Mail with some App statistics
*/
@Injectable()
export class Feedback {

	constructor(
		private emailComposer: EmailComposer,
		private dao: DAO,
		public translate: TranslateService
	){}

	/**
	* Check if Mail Client is available on user Device.
	* @returns resolves true
	*/
	public mailAvailable(){
		return Promise.resolve(true); //TODO
	}

	/**
	* Starts Mail Activity with Feedback Template.
	*/
	public openMail(){
		this.dao.getPosterIDs().then((ids:Array<any>)=>{
			this.translate.get(['email_BODY','email_SUBJECT']).subscribe( t => {
				let email = {
					to: 'app@postersafari.info',
					subject: t.email_SUBJECT,
					body: t.email_BODY+ '\nversion: 0.0.9\nimage ids:\n' + ids.toString(),
					isHtml: false
				};
				this.emailComposer.open(email);
			});
		});
	}
}
