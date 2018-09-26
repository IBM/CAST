import _ from 'lodash';
import PropTypes from 'prop-types';
import React, { Component } from 'react';
import {getTitle} from '../../search_utils';
import {CastFieldSelect} from './cast_field_select';
import {CastIndexSelect} from './cast_index_select';

import {
    EuiButtonIcon,
    EuiPanel,
    EuiAccordion,
    EuiForm
} from '@elastic/eui';

export class CastSearchEditor extends Component 
{ 
    constructor(props){
        super(props);
        this.state = {
            fieldList : [],
            fieldMap  : {},
        };
        // Set the defaults.
        this.displayFieldCache = {
            "": [] 
        };
        this.displayFieldCache[this.props.searchParams.indexPattern] =  this.props.searchParams.displayFields;
        this.props.searchParams.displayFields = [];        

        this.collisionFieldCache = {
            "" :[]
        }

        this.displayFieldSelection = [];
        this.activeIndexId = "";
    }

    componentDidMount = () => {
        this.props.getIndexPatterns(this.props.searchParams.indexPattern).then((savedObjects) => {
            console.log(savedObjects)
            savedObjects.some( (obj) => {
                if ( obj.attributes.title  === this.props.searchParams.indexPattern )
                {
                    this.onIndexChange({
                        label: obj.attributes.title,
                        value: obj.id
                    });
                    return true;
                }
                return false;
            });
        });
    }

    onIndexChange = (evt) => { 
        this.props.searchParams.indexPattern = evt.label;
        this.props.searchParams.indexPatternId = evt.value;
        this.props.handleSearchUpdate(this.props.searchIndex, this.props.searchParams);

        if (evt.value != this.activeIndexId)
        {
            // Cache the user selections to restore them in the event of a mistake.
            this.displayFieldCache[this.activeIndexId] = this.props.searchParams.displayFields;
            this.props.searchParams.displayFields = _.get(this.displayFieldCache, evt.label, []);
            this.displayFieldSelection = 
                this.props.searchParams.displayFields.map( (key) => { return { label : key, value : key }});

            this.props.handleSearchUpdate(this.props.searchIndex, this.props.searchParams);

            // Update the active index id.
            this.activeIndexId = evt.value;
            
            var newFieldMap = {};
            // Get the fields from the new index, updating the state when done.
            this.props.getIndexPattern(evt.value).then( (pattern) => { 
                const newFieldList = _.get(pattern, "fields", []).map( (field) => {
                    newFieldMap[field.displayName] = false; 
                    return {
                        label : field.displayName,
                        value : field.displayName
                    };
                });
                this.setState( prevState  => ({fieldList : newFieldList, fieldMap : newFieldMap}));

            });
        }
    }

    onFieldSelectChange = (evt) => {
        if (evt)
        {
            this.props.searchParams.displayFields = evt.map( (key) => { return key; });
            this.displayFieldSelection  = evt;
        }
        else
        {
            this.displayFieldSelection  = evt;
            this.props.searchParams.displayFields = [];
        }
        this.props.handleSearchUpdate(this.props.searchIndex, this.props.searchParams);
    }


    removeSearch = () => {
        this.props.onRemove(this.props.searchIndex);
    }

    renderFieldSelect = () => {
        //fieldList={_.get(this.props, "searchParams.displayFields", [])}
        return     }

    renderControls = () => {
        return (
            <EuiForm>
                <CastIndexSelect 
                    onChange={this.onIndexChange}
                    controlIndex={this.props.searchIndex}
                    value={this.props.searchParams.indexPatternId}
                    getIndexPatterns={this.props.getIndexPatterns}
                />
                <CastFieldSelect
                    value={this.displayFieldSelection}
                    searchIndex={this.props.searchIndex}
                    fieldList={this.state.fieldList}
                    onChange={this.onFieldSelectChange}
                />
                {this.renderFieldSelect()}
            </EuiForm>
        );
    }


    renderTopButtons() {
        return (
            <div>
              <EuiButtonIcon
                aria-label="Remove Search"
                color="danger"
                onClick={this.removeSearch}
                iconType="cross"
              />
            </div>
        );
    }

    render() {
        return(
          <div> 
           <EuiPanel grow={false} className="controlEditorPanel">
            <EuiAccordion
                id="searchEditorAccordion"
                buttonContent={getTitle(this.props.searchParams, this.props.searchIndex)}
                extraAction = { this.renderTopButtons()}
                initialIsOpen={false}>

                {this.renderControls()} 
            </EuiAccordion>
           </EuiPanel>
          </div>);
    }
}

CastSearchEditor.propTypes = {
    onRemove: PropTypes.func.isRequired,
    searchIndex: PropTypes.number.isRequired,
    searchParams: PropTypes.object.isRequired,
    handleSearchUpdate: PropTypes.func.isRequired,
    getIndexPatterns: PropTypes.func.isRequired,
    getIndexPattern: PropTypes.func.isRequired
};
