

import PropTypes from 'prop-types';
import React, { Component } from 'react';
import _ from "lodash";

import {
    EuiFieldNumber,
    EuiForm,
    EuiFormRow,
    EuiButton,
    EuiFlexGroup,
    EuiFlexItem
} from '@elastic/eui';


export class AllocationComponent extends Component
{
    constructor(props){
        super(props);
        this.props.search.allocation_id = 0;
        this.handleSubmit = this.handleSubmit.bind(this);
        this.handleAllocationChange = this.handleAllocationChange.bind(this);
        this.handleResponse = this.handleResponse.bind(this);


        //this.
    }

    handleSubmit()
    {
       var query = { 
        bool : { 
         should : [ 
          { match : 
           { "data.allocation_id" :  this.props.search.allocation_id } }
        ] } }  ;

        this.props.onSearch(query, this.props.search, this.handleResponse);
    }

    handleResponse(resp)
    {
        const hits = resp.hits;

        if (hits.total === 1)
        {
            const allocation = _.get( hits, "hits[0]._source");
            const filter = {
                query : {
                 multi_match : {
                  query :  allocation.data.compute_nodes.join(" "),
                  type : "best_fields",
                  fields : [ "hostname", "source" ],
                  tie_breaker : 0.3,
                  minimum_should_match : 1
                 }
                }
            };

            var end_time = allocation.data.history ? allocation.data.history.end_time : null;
            this.props.handleTimeRange(allocation.data.begin_time, end_time);
            this.props.handleFilter(filter, this.props.search.id);
            //console.log("Allocation Data: " , allocation.data);
            //console.log("Display Fields: " , this.props.search.displayFields);
            //console.log("Allocation Fields: " , _.get( allocation, this.props.search.displayFields));
            //console.log("Props Search: " , this.props.search);
            var results = {}
            this.props.search.displayFields.map( (key) => {
                const value = _.get(allocation, key, false);
                if ( value )
                {
                    results[key] = value;
                }
            });
            this.props.renderResults(results);
        }

        // TODO fail.
    }

    handleAllocationChange(event) {
        this.props.search.allocation_id = event.target.value;
    }

    render() {
        return (
           <EuiFlexItem grow={false}>
           <EuiFormRow
            id={this.props.search.id}
            label={this.props.search.type}
            >
                <EuiFieldNumber
                    label="Allocation ID"
                    placeholder={this.props.search.allocation_id}
                    onChange={this.handleAllocationChange}
                    name="allocation-id"/>
            </EuiFormRow>

             <EuiFormRow>
              <EuiButton
               onClick={this.handleSubmit}>
                  Apply Changes
              </EuiButton>
             </EuiFormRow>
            </EuiFlexItem>
        );
        
    }

}

AllocationComponent.propTypes = {
    search: PropTypes.object.isRequired,
    searchIndex: PropTypes.number.isRequired,
    onSearch: PropTypes.func.isRequired,
    handleFilter: PropTypes.func.isRequired,
    renderResults: PropTypes.func.isRequired,
    handleTimeRange: PropTypes.func.isRequired
}
