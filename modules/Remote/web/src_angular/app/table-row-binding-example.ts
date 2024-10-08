import {Component, OnInit} from '@angular/core';
import {webSocket, WebSocketSubject} from 'rxjs/webSocket';
import { Router } from '@angular/router';
import { int } from '@kitware/vtk.js/types';
import {MatDialog} from '@angular/material/dialog';
import {DialogComponent} from './dialog/dialog.component'


export interface caption {
  id:int;
  Title: string;
  x: int;
  y: int;
  z: int;
  hide:Boolean;
}

const ELEMENT_DATA: caption[] = [

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
  displayedColumns: string[] = ['id', 'Title', 'jump', 'delete', 'edit', 'hide'];
  dataSource = ELEMENT_DATA;
  clickedRows = new Set<caption>();
  href: string = "asdf";
  WebSocket: WebSocketSubject<any>;
  addMode: Boolean = false;
  selectedID: int = -1;
  buttonColor:string ='primary';

  constructor(public dialog: MatDialog) {}

  ngOnInit() {
    //this.href = this.router.url;
    console.log("this.router.url");
    this.WebSocket = webSocket('ws://'+location.hostname+':1234');
    this.WebSocket. subscribe(messages => {
		if (messages.id ==="caption.response") {
			this.dataSource=messages.captionList;
			this.addMode=false;
		}
		if (messages.id ==="caption.interactionUpdate") {
			this.selectedID=messages.focusedId;
		}
	});
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
    this.addMode=true;
    let  methode:String = "addMode.caption";
    this.WebSocket.next({method: methode, addMode:this.addMode } );
  }

  hideAnnotation(id: int){
    let  methode:String = "hideAnnotation.caption";
    this.WebSocket.next({method: methode, id:id } );
  }

  openDialog(id:int): void {
    let title = "";
    for (var element of this.dataSource) {
      if (element.id == id) {
        title = element.Title;
      }
    }
    const dialogRef = this.dialog.open(DialogComponent, {
      width: '250px',
      data: {title: title},
    });
    dialogRef.afterClosed().subscribe(result => {
      if (result) {
        let  methode:String = "nameChanged.caption";
        this.WebSocket.next({method: methode, id:id , title:result} );
      }
    });
  }
}
