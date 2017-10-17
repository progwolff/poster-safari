/*
* Global utilility functions
*/
export namespace Util {

	/**
	* efficent in memory sorting of array
	* taken from https://stackoverflow.com/a/26759127
	* uses array.sort
	* @param array: the Array of elements
	* @param o.prop: property name (if it is an Array of objects)
	* @param o.desc: determines whether the sort is descending
	* @param o.parser: function to parse the items to expected type
	* @return sorted array
	*/
	export var sortBy = (function () {

		//cached privated objects
		var _toString = Object.prototype.toString,
		//the default parser function
		_parser = function (x) { return x; },
		//gets the item to be sorted
		_getItem = function (x) {
			return this.parser((x !== null && typeof x === "object" && x[this.prop]) || x);
		};

		/**
		* Creates a method for sorting the Array
		* @param array: the Array of elements
		* @param o.prop: property name (if it is an Array of objects)
		* @param o.desc: determines whether the sort is descending
		* @param o.parser: function to parse the items to expected type
		*/
		return function (array, o) {
			if (!(array instanceof Array) || !array.length){
				return [];
			}
			if (_toString.call(o) !== "[object Object]"){
				o = {};
			}
			if (typeof o.parser !== "function"){
				o.parser = _parser;
			}
			o.desc = !!o.desc ? -1 : 1;
			return array.sort(function (a, b) {
				a = _getItem.call(o, a);
				b = _getItem.call(o, b);
				return o.desc * (a < b ? -1 : +(a > b));
			});
		};
	}());

	/**
	* Generates random uuid. Format of rfc4122 (8-4-4-4-12), but not compliant.
	@returns UUID
	*/
	export function uuid():string{
		return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function(c) {
			var r = Math.random()*16|0, v = c == 'x' ? r : (r&0x3|0x8);
			return v.toString(16);
		});
	}

	/**
	* Converts a datablob to base64 encoded string.
	* @param blob datablob
	* @returns string representing blob in base64
	*/
	export function blobToBase64(blob):Promise<string> {
		return new Promise<string>((resolve, reject) => {
			var reader = new FileReader();
			reader.onload = function() {
				var dataUrl:string = reader.result;
				var base64:string = dataUrl;
				if(dataUrl.includes(',')) {
					base64 = dataUrl.split(',')[1];
				}
				resolve(base64);
			};
			reader.readAsDataURL(blob);
		});
	}
}
