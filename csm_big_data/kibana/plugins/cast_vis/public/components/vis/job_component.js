

import PropTypes from 'prop-types';
import React, { Component } from 'react';
import _ from "lodash";
import CollisionModal from './collision_modal';

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
        this.handleCollisionModal = this.handleCollisionModal.bind(this);


        this.state = {
            showCollisionModal: false
        };

        this.responseObject={}
    }

    handleSubmit()
    {
       var query = { 
        bool : { 
         should : [ 
          { match : 
           { "data.primary_job_id" :  this.props.search.primary_job_id } },
          { match : 
           { "data.secondary_job_id" :  this.props.search.secondary_job_id } }
          ] ,
          "minimum_should_match" : 2
        } }  ;

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

            end_time = allocation.data.history ? allocation.data.history.end_time : null;
            this.props.handleTimeRange(allocation.data.begin_time, end_time);
            this.props.handleFilter(filter, this.props.search.id);

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
        else if ( hits.total > 0 )
        {
            this.responseObject = hits.hits;
            this.setState({ isModalVisible: true });
        }

        // TODO fail.
    }

    handlePrimaryChange(event) {
        this.props.search.primary_job_id = event.target.value;
    }

    handleSecondaryChange(event) {
        this.props.search.secondary_job_id = event.target.value;
    }
    
    handleCollisionModal(resolution) {
        if (resolution)
        {
            this.handleResponse({ 
                hits: {
                    hits: [ resolution ],
                    total : 1
                }
            });
        }
        this.setState({ isModalVisible: false });
    }

    render() {
        let modal;
        if (this.state.isModalVisible)
        {
            modal = (
                <CollisionModal
                    onClose={this.handleCollisionModal}
                    responseObj={this.responseObject}
                    collisionFilter={["data.allocation_id", "data.user_name", "data.begin_time", "data.history.end_time",  "data.state"]}
                />
            );
        }
        return (
           <div>
           <EuiFlexItem grow={false}>
             <EuiFormRow
                id={this.props.search.id}
                label={this.props.search.type}
                >
                <EuiFieldNumber
                    label="Primary Job ID"
                    placeholder={this.props.search.primary_job_id}
                    onChange={this.handlePrimaryChange}
                    name="primary-job-id"/>

            </EuiFormRow>
             <EuiFormRow>
                <EuiFieldNumber
                    label="Secondary Job ID"
                    placeholder={this.props.search.secondary_job_id}
                    onChange={this.handleSecondaryChange}
                    name="secondary-job-id"/>
            </EuiFormRow>

             <EuiFormRow>
              <EuiButton
               onClick={this.handleSubmit}>
                  Apply Changes
              </EuiButton>
             </EuiFormRow>
            </EuiFlexItem>

            {modal}
            </div>
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
