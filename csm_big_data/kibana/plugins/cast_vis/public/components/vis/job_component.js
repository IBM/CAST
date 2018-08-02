import PropTypes from 'prop-types';
import React, { Component } from 'react';


import {
    EuiFieldNumber,
    EuiForm,
    EuiFormRow,
    EuiButton,
    EuiFlexGroup,
    EuiFlexItem
} from '@elastic/eui';


export class JobComponent extends Component
{
    constructor(props){
        super(props);
        this.props.search.primary_job_id = 0;
        this.props.search.secondary_job_id = 0;

        this.handleSubmit = this.handleSubmit.bind(this);
        this.handlePrimaryChange = this.handlePrimaryChange.bind(this);
        this.handleSecondaryChange = this.handleSecondaryChange.bind(this);
        this.handleResponse = this.handleResponse.bind(this);
    }

    handleSubmit()
    {
       var query = { 
        bool : { 
         should : [ 
          { match : 
           { "data.primary_job_id" :  this.props.search.primary_job_id } }
           { "data.secondary_job_id" :  this.props.search.secondary_job_id } }
        ] } }  ;

        this.props.onSearch(query, this.props.search, this.handleResponse);
    }

    handleResponse(resp)
    {
        const hits = resp.hits;

        if (hits.total === 1)
        {
            console.log(hits);
            const allocation = hits.hits[0]._source;
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
            this.props.handleTimeRange(allocation.data.begin_time, allocation.data.history.end_time);
            this.props.handleFilter(filter, this.props.search.id);
            this.props.renderResults(allocation.data);
        }
        console.log("multiple records were detected!");

        // TODO fail.
    }

    handlePrimaryChange(event) {
        this.props.search.primary_job_id = event.target.value;
    }

    handleSecondaryChange(event) {
        this.props.search.secondary_job_id = event.target.value;
    }

    render() {
        return (
           <EuiFlexItem grow={false}>
           <EuiFormRow
            id={this.props.search.id}
            label={this.props.search.type}
            >
                <EuiFieldNumber
                    placeholder={this.props.search.primary_job_id}
                    onChange={this.handlePrimaryChange}
                    name="Primary Job ID"/>
                <EuiFieldNumber
                    placeholder={this.props.search.secondary_job_id}
                    onChange={this.handleSecondaryChange}
                    name="Secondary Job ID"/>
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

JobComponent.propTypes = {
    search: PropTypes.object.isRequired,
    searchIndex: PropTypes.number.isRequired,
    onSearch: PropTypes.func.isRequired,
    handleFilter: PropTypes.func.isRequired,
    renderResults: PropTypes.func.isRequired,
    handleTimeRange: PropTypes.func.isRequired
}
