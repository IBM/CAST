import React from 'react';
import { render, unmountComponentAtNode } from 'react-dom';
import {CastSearchVis} from "./components/vis/cast_search_vis";

import _ from 'lodash';


class CastVisController{
    constructor(el, vis) {
        this.el = el;
        this.vis = vis;
        this.searches = [];
    }

    async render(visData, status) {
        this.drawSearches();
        return;
    }

    destroy() {
        unmountComponentAtNode(this.el);
    }

    drawSearches(){ 
        render(
            <CastSearchVis
             searches={this.vis.params.searches}
             defaultIndex={this.vis.params.defaultIndex}
             scope={this.vis}
            />,
            this.el);
    }
}


export { CastVisController };
