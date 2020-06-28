let obj_base = {
  text: "salut",
  m: function() { log(this.text); }
}

let _text = obj_base.text;
let new_obj = Object.create(obj_base);

Object.assign(new_obj, {
  set text(new_text) {
    _text = new_text
  },
  get text() {
    return _text;
  }
});

new_obj.text = "patate";

log(new_obj.text);
new_obj.m();