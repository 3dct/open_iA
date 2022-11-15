import {Component, OnInit} from '@angular/core';
import {webSocket, WebSocketSubject} from 'rxjs/webSocket';
import { Router } from '@angular/router';
import { int } from '@kitware/vtk.js/types';


export interface PeriodicElement {
  id:int;
  Title: string;
  x: int;
  y: int;
  z: int;
}

const ELEMENT_DATA: PeriodicElement[] = [
  {id: 1, Title: 'Hydrogen', x: 1.0079, y: 1.0079, z: 1.0079},
  {id: 2, Title: 'Helium', x: 1.0079, y: 1.0079, z: 1.0079},
  {id: 3, Title: 'Lithium', x: 1.0079, y: 1.0079, z: 1.0079},
  {id: 4, Title: 'Beryllium', x: 1.0079, y: 1.0079, z: 1.0079},
  {id: 5, Title: 'Boron', x: 1.0079, y: 1.0079, z: 1.0079},
  {id: 6, Title: 'Carbon', x: 1.0079, y: 1.0079, z: 1.0079},
  {id: 7, Title: 'Nitrogen', x: 1.0079, y: 1.0079, z: 1.0079},
  {id: 8, Title: 'Oxygen', x: 1.0079, y: 1.0079, z: 1.0079},
  {id: 9, Title: 'Fluorine', x: 1.0079, y: 1.0079, z: 1.0079},
  {id: 10, Title: 'Neon', x: 1.0079, y: 1.0079, z: 1.0079},
];

/**
 * @title Binding event handlers and properties to the table rows.
 */
@Component({
  selector: 'table-row-binding-example',
  styleUrls: ['table-row-binding-example.css'],
  templateUrl: 'table-row-binding-example.html',
})
export class TableRowBindingExample {
  displayedColumns: string[] = ['position', 'name', 'weight', 'symbol'];
  dataSource = ELEMENT_DATA;
  clickedRows = new Set<PeriodicElement>();
  href: string = "asdf";
  WebSocket: WebSocketSubject<any>;
  addMode: Boolean = false;
  selectedID: int = -1;

  buttonColor:string ='primary';

  constructor() {}

  ngOnInit() {
    //this.href = this.router.url;
    console.log("this.router.url");
    this.WebSocket = webSocket('ws://localhost:1234');
    this.WebSocket. subscribe(messages => this.dataSource=messages.captionList );
    let  methode:String = "subscribe.captions";
    this.WebSocket.next({method: methode } );

}

sendDelete(id: int){
    let  methode:String = "remove.caption";
    this.WebSocket.next({method: methode, id:id } );
}

sendSelect(id: int){
  let  methode:String = "select.caption";
  this.WebSocket.next({method: methode, id:id } );
  this.selectedID = id;
}

addModeToggle(){
  let  methode:String = "addMode.caption";
  this.addMode = !this.addMode;
  this.WebSocket.next({method: methode, addMode:this.addMode } );
}



}


/**  Copyright 2022 Google LLC. All Rights Reserved.
    Use of this source code is governed by an MIT-style license that
    can be found in the LICENSE file at https://angular.io/license */