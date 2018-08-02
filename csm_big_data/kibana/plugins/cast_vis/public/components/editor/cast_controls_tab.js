import _ from 'lodash';
import React, { Component } from 'react';
import PropTypes from 'prop-types';
import { addSearch, newSearch, removeSearch} from '../../search_utils';

import {CastSearchEditor} from './cast_search_editor.js';

//import {CastIndexSelect} from './cast_index_select';
//
import {
  EuiButton,
  EuiFlexGroup,
  EuiFlexItem,
  EuiFormRow,
  EuiPanel,
  EuiSelect,
} from '@elastic/eui';

export class CastControlsTab  extends Component {
    state = {
        type: 'allocation-id',
        indexID: ''
    }
    
    constructor(props){
        super(props);

        this.getIndexPatterns("cast-allocation").then( 
            (savedObjects) => {
                
                for (var idx in savedObjects)
                {
                    if(savedObjects[idx].attributes.title === "cast-allocation")
                    {
                        return this.setVisParam('defaultIndex', savedObjects[idx].id);
                    }
                }
            });
    }


    initIndices = async () => {
        const patterns = await this.getIndexPatterns("cast-allocation");    
        console.log(patterns);

    }


    /**
     *
     * @param {string} search A search string containing the index pattern to search for.
     *
     * @returns {array} An array of saved objjects containing the index patterns.
     */
    getIndexPatterns = async (search) => {
        const resp = await this.props.scope.vis.API.savedObjectsClient.find({
            type: 'index-pattern',
            fields: ['title'],
            search: `${search}`,
            search_fields: ['title'],
            perPage: 100
        });
        console.log("it worked");
        console.log(resp);
        return resp.savedObjects;
    }

    getIndexPattern = async (indexPatternId) => {
        return await this.props.scope.vis.API.indexPatterns.get(indexPatternID);
    }

    // Copied from control tabs
    setVisParam(paramName, paramValue) {
        const params = _.cloneDeep(this.props.scope.vis.params);
        params[paramName] = paramValue;
        this.props.stageEditorParams(params);
    }

    changeIndexPattern = (event) => {
        console.log(event.value);
        //this.props.handleIndexPatternChange(this.props.controlIndex, event);
    }

    handleAddSearch = () => {
        this.setVisParam('searches', addSearch( this.props.scope.vis.params.searches, newSearch(this.state.type)));
    }

    handleRemoveSearch = (searchIndex) => {
        this.setVisParam('searches', removeSearch( this.props.scope.vis.params.searches, searchIndex));
    }
    
    renderSearches() {
        return this.props.scope.vis.params.searches.map((searchParams, searchIndex) => {
            return ( 
                <CastSearchEditor
                    key={searchParams.id}
                    onRemove={this.handleRemoveSearch}
                    searchIndex={searchIndex}
                    searchParams={searchParams}
                />
                );
        });
    }


    render() {
        return (
           <div>
            {this.renderSearches()}
            <EuiPanel grow={false}>
              <EuiFlexGroup>
               <EuiFlexItem>
                <EuiFormRow id="searchTypeSelector">
                 <EuiSelect
                   options={[
                    { value: 'allocation-id', text: 'Allocation ID' },
                    { value: 'job-id', text: 'Job ID' },
                    { value: 'custom', text: 'Custom Search' },
                    ]}
                   value={this.state.type}
                   onChange={evt => this.setState({ type: evt.target.value })}
                   aria-label="Select search type"
                  />
                </EuiFormRow>
               </EuiFlexItem>
               <EuiFlexItem>
                <EuiFormRow id="addContol">
                 <EuiButton
                  fill
                  onClick={this.handleAddSearch}
                  iconType="plusInCircle"
                  aria-label="Add control"
                  > 
                    Add
                  </EuiButton>
                </EuiFormRow>
               </EuiFlexItem>
              </EuiFlexGroup>
             </EuiPanel>
           </div>
        );
    }
}

CastControlsTab.propTypes = {
    scope: PropTypes.object.isRequired,
    stageEditorParams: PropTypes.func.isRequired
};


