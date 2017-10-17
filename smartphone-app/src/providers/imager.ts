import { Injectable } from '@angular/core';
import { File, FileEntry } from '@ionic-native/file';

import { Util } from './util';

/**
* Processes images including orientation setting, thumbnail generation, encoding
*/
@Injectable()
export class Imager {

	constructor(private file: File) {}

	/**
	* convert imageURI to base64 strings; both full image and scaled down thumb.
	* @param imageURI fileuri to image
	* @return [fullImage, thumbnail]
	*/
	public getImages(imageURI: string):Promise<Array<string>> {
		return new Promise((resolve, reject) => {
			this.file.resolveLocalFilesystemUrl(imageURI).then((fileEntry) => {
				(<FileEntry>fileEntry).file(file => {

					var reader = new FileReader();
					reader.onloadend = () => {
						var orientation:number = this.getOrientation(new DataView(reader.result));
						//get a base64 Thumbnail from image
						var thumbPromise = this.createThumbnail(imageURI, 640).then((thumb)=>{
							return this.resetOrientation(thumb,orientation).then(Util.blobToBase64);
						});
						//get image as base64
						var blob:Blob = new Blob([new Uint8Array(reader.result)], { type: "image/jpeg" });
						var imagePromise = Util.blobToBase64(blob);
						resolve(Promise.all([imagePromise, thumbPromise]));
					};//end reader.onloaded
					reader.readAsArrayBuffer(file);
				}); // end file

			}).catch((err) => { //end resolveLocalFilesystemUrl
				console.log("could not get a file entry for captured image file"+err);
				reject();
				// If don't get the FileEntry (which may happen when testing
				// on some emulators), copy to a new FileEntry.
				//TODO: createNewFileEntry(imgUri);, see https://cordova.apache.org/docs/en/latest/reference/cordova-plugin-camera/#take-a-picture-
			}); //end catch
		}); //end promise
	}//end function

	/**
	* extract orientation from image
	* from https://stackoverflow.com/a/32490603
	* @param view image as DataView
	* @returns encoded Orientation. 1-8 valid orientations, <0 errors
	*/
	private getOrientation(view):number{
		if (view.getUint16(0, false) != 0xFFD8) {
			console.log("not a jpeg");
			return -2;

		}
		var length = view.byteLength, offset = 2;
		while (offset < length) {
			var marker = view.getUint16(offset, false);
			offset += 2;
			if (marker == 0xFFE1) {

				if (view.getUint32(offset += 2, false) != 0x45786966) {
					console.log("orientation not defined");
					return -1;
				}

				var little = view.getUint16(offset += 6, false) == 0x4949;
				offset += view.getUint32(offset + 4, little);
				var tags = view.getUint16(offset, little);
				offset += 2;

				for (var i = 0; i < tags; i++) {
					if (view.getUint16(offset + (i * 12), little) == 0x0112) {
						return view.getUint16(offset + (i * 12) + 8, little);
					}
				}

			} else if ((marker & 0xFF00) != 0xFF00){
				break;
			} else {
				offset += view.getUint16(offset, false);
			}
		} // end while
		return -1;
	} // end function

	/**
	* Creates scaled image
	* @param img imageURI of big image
	* @param MAX_WIDTH maximum width in px
	* @param MAX_HEIGHT maximum height in px
	*/
	private createThumbnail(img, MAX_WIDTH: number = 900, MAX_HEIGHT: number = 900) {
		return new Promise((resolve, reject) => {
			var image = new Image();
			image.onload = function () {
				var width = image.width;
				var height = image.height;
				var canvas = document.createElement('canvas');
				//scale width/height
				if (width > height) {
					if (width > MAX_WIDTH) {
						height *= MAX_WIDTH / width;
						width = MAX_WIDTH;
					}
				} else {
					if (height > MAX_HEIGHT) {
						width *= MAX_HEIGHT / height;
						height = MAX_HEIGHT;
					}
				}
				canvas.width = width;
				canvas.height = height;
				var ctx = canvas.getContext("2d");
				ctx.drawImage(image, 0, 0, width, height);
				resolve(canvas.toDataURL('image/jpeg'));

			}; // end onload
			image.src = img;
		});//end promise
	}

	/**
	* sets orientation into base64 encoded image
	* from https://stackoverflow.com/a/40867559
	* @param base64img image in base64 encoding
	* @param srcOrientation orientation to be set
	* @returns base64 image with orientation
	*/
	public resetOrientation(base64img, srcOrientation) {
		var img = new Image();
		var imagePromise = new Promise(function(resolve,reject){
			img.onload = function() {
				var width = img.width;
				var height = img.height;
				var canvas = document.createElement('canvas');
				var ctx = canvas.getContext("2d");

				// set proper canvas dimensions before transform & export
				if ([5,6,7,8].indexOf(srcOrientation) > -1) {
					canvas.width = height;
					canvas.height = width;
				} else {
					canvas.width = width;
					canvas.height = height;
				}

				// transform context before drawing image
				switch (srcOrientation) {
					case 2: ctx.transform(-1, 0, 0, 1, width, 0); break;
					case 3: ctx.transform(-1, 0, 0, -1, width, height ); break;
					case 4: ctx.transform(1, 0, 0, -1, 0, height ); break;
					case 5: ctx.transform(0, 1, 1, 0, 0, 0); break;
					case 6: ctx.transform(0, 1, -1, 0, height , 0); break;
					case 7: ctx.transform(0, -1, -1, 0, height , width); break;
					case 8: ctx.transform(0, -1, 1, 0, 0, width); break;
					default: ctx.transform(1, 0, 0, 1, 0, 0);
				}

				// draw image
				ctx.drawImage(img, 0, 0);
				canvas.toBlob((blob) => resolve(blob), 'image/jpeg');
			};
		});
		img.src = base64img;
		return imagePromise;
	}
}
