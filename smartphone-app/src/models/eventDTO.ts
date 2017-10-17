import * as moment from 'moment';
/**
* The poster with its extracted event
*/
export class EventDTO {
	_id: string;
	_rev: string;
	readonly version: string  = "0.2";
	created: Date;
	event:string;
	serverEvent: PostrEvent;
	userEvent: PostrEvent;
	_attachments: Attachments;
	constructor(id?:string) {
		if(id !== undefined){
			this._id = id;
		}
		this.userEvent = new PostrEvent();
		this.created = new Date();
		this._attachments = new Attachments();
	}

	/**
	* get variable in a save way without triggering a crash
	*/
	getEventVar(variableName:string){
			var value;
			if(this.serverEvent && this.serverEvent[variableName]){
				value = this.serverEvent[variableName];
			}
			if(this.userEvent && this.userEvent[variableName] && this.userEvent[variableName].toString().length != 0){
				value = this.userEvent[variableName];
			}
			return value;
	}

	hasImageThumb():boolean{
		return	this._attachments !== undefined &&
						this._attachments.userimage_thumb !== undefined &&
						this._attachments.userimage_thumb.data !== undefined &&
						this._attachments.userimage_thumb.data !== "";
	}

	hasImage():boolean{
		return 	this._attachments !== undefined &&
						this._attachments.userimage !== undefined &&
						this._attachments.userimage.data !== undefined &&
						this._attachments.userimage.data !== "";
	}

	setImage(image){
		if(this._attachments === undefined){
			this._attachments = new Attachments();
		}
		this._attachments.setImage(image);
	}

	setImageThumb(thumb){
		if(this._attachments === undefined){
			this._attachments = new Attachments();
		}
		this._attachments.setImageThumb(thumb);
	}

	/**
	* @return thumb or if not available fullimage
	*/
	getPicture(){
		if(this.hasImageThumb()){
			return "data:image/jpeg;base64,"+this._attachments.userimage_thumb.data;
		}else{
			return this.getFullSizePicture();
		}
	}

	getFullSizePicture(){
		if(this.hasImage()){
			return "data:image/jpeg;base64,"+this._attachments.userimage.data;
		}else{
			return undefined;
		}
	}

	/**
	* test image getters and setters
	*/
	test(verbose:boolean):boolean{
		var success = true;
		var test:EventDTO = new EventDTO();
		if(verbose){
			var desc:string = "empty";
			console.log(desc + " DTO" + JSON.stringify(test));
			console.log(desc + " DTO hasImage=" + test.hasImage() + " hasImageThumb="+ test.hasImageThumb());
		}
		success = success && test.hasImage() == false && test.hasImageThumb() == false;

		test.setImageThumb("Thumb");
		success = success && test.hasImage() == false && test.hasImageThumb() == true;
		if(verbose){
			var desc:string = "thumb";
			console.log(desc + " DTO" + JSON.stringify(test));
			console.log(desc + " DTO hasImage=" + test.hasImage() + " hasImageThumb="+ test.hasImageThumb());
		}

		test.setImage("Image");
		success = success && test.hasImage() == true && test.hasImageThumb() == true;
		if(verbose){
			var desc:string = "thumb+image";
			console.log(desc + " DTO" + JSON.stringify(test));
			console.log(desc + " DTO hasImage=" + test.hasImage() + " hasImageThumb="+ test.hasImageThumb());
		}

		test.setImage("Image2");
		success = success && test.hasImage() == true && test.hasImageThumb() == true;
		if(verbose){
			var desc:string = "thumb+imageOverridden";
			console.log(desc + " DTO" + JSON.stringify(test));
			console.log(desc + " DTO hasImage=" + test.hasImage() + " hasImageThumb="+ test.hasImageThumb());
		}
		return success;
	}
}

/**
* Representation of Attachments in dto.
* can hold the fullimage and thumb.
*/
export class Attachments {
	userimage: Attachment;
	userimage_thumb: Attachment;

	constructor() {
		this.userimage = new Attachment();
		this.userimage_thumb = new Attachment();
	}

	setImage(image){
			this.userimage = new Attachment(image,"image/jpeg");
	}

	setImageThumb(thumb){
			this.userimage_thumb = new Attachment(thumb,"image/jpeg");
	}
}

/**
* Attachment that is registered by pouchdb as such
*/
export class Attachment {
	content_type: string;
	data: string;
	constructor(data:any = "", contentType:string = "text/plain") {
		this.data = data;
		this.content_type = contentType;
	}
}

/**
* Represents a event. used as serverEvent and userEvent
*/
export class PostrEvent {
	_id:string;
	_ref:string;
	readonly version: "0.2";
	title: string;
	description: string;
	artists:Array<string>;
	organizers: Array<string>;
	times: Array<Time>;
	tickets:Array<Ticket>;
	venue_name: string;
	venue_type: string;
	address: string;
	city: string;
	region: string;
	postal_code: string;
	country: string;
	latitude: number;
	longitude: number;
	enabled:boolean;
	urls:Array<URL>;
	ticketURL:URL;

	constructor() {}

	/**
	* get adress as string
	*/
	getAddress():string{
		let address = "";

		if(this.address)
			address += this.address;

		if(this.city){
			if(this.address)
				address += ", ";
			if(this.postal_code)
				address += this.postal_code+" ";
			address += this.city;
		}

		return address;
	}

	/**
	* get date as string
	* @param t timeobject
	* @param startNotStop startDate or stopDate?
	*/
	getDate(t:Time, startNotStop:boolean=true):string{
		let format = "DD.MM.YYYY, HH:mm [Uhr]";
		if(t.allDay)
			format = "DD.MM.YYYY";

		let d = t.start;
		if(!startNotStop)
			d = t.stop;

		return moment(d).format(format);
	}

	/**
	* get some string representing the event venue.
	* @return if available in this order venue > adress > title
	*/
	getVenueName():string{
		if(this.venue_name)
			return this.venue_name;
		if(this.address)
			return this.getAddress();
		if(this.title)
			return this.title;
	}

	/**
	* get nice string representing the event
	* contains title, time, venue, city, description
	*/
	beautify():string{
		let s = this.title;
		if(this.times && this.times[0].start)
			s += "\n"+this.getDate(this.times[0])+"\n";
		if(this.venue_name)
			s += "\n"+this.venue_name;
		if(this.city)
			s += "\n"+this.getAddress();
		if(this.description)
			s += "\n\n"+this.description;
		return s;
	}
}
/**
* Represents a event-time
*/
export class Time {
	start:Date;
	stop:Date;
	allDay:boolean;
}

/**
* Represents ticket type in a event
*/
export class Ticket{
	price:number;
	description:string;
	currency:string;
}
