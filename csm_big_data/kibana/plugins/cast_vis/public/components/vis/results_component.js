import PropTypes from 'prop-types';
import React, { Component } from 'react';


import {
    EuiFlexItem,
      EuiInMemoryTable,
      EuiBasicTable,
} from '@elastic/eui';

export class ResultsComponent extends Component
{
    constructor(props){
        super(props);
        
        this.state = {
            columns : [
                { field : "x", name: 'Attribute', truncateText: false, sortable: true },
                { field : "y", name: 'Value', truncateText: false, sortable: false}],
            items   : [],
        }
    }
    
    updateResults(resultsMap, isMultiple){
        this.setState( prevState => ({ items : Object.keys(resultsMap).map( key => ({x : key, y : resultsMap[key] }))}));
    }

    render(){
        return (
        <EuiInMemoryTable
            items={this.state.items} 
            columns={this.state.columns}
        />
        );
    }

}
