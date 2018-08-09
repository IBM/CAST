import PropTypes from 'prop-types';
import React, { Component } from 'react';
import Select from 'react-select';


import { 
    EuiFormRow,
} from '@elastic/eui';


export class CastIndexSelect extends Component
{
    constructor(props){
        super(props);
        this.loadOptions = this.loadOptions.bind(this);
    }

    // Load the options list from the index patterns.
    loadOptions(input, callback) {
        this.props.getIndexPatterns(input + "*").then((savedObjects) => {
            const idxList = savedObjects.map( (obj) => {
                return {
                     label: obj.attributes.title,
                     value: obj.id
                 };
             });

            callback(null, {options:idxList});
        });
    }

    render() { 
        const cId = `castIndexSelect-${this.props.controlIndex}`;
        return (
            <EuiFormRow
                id={cId}
                label="Index Pattern"
            >
              <Select.Async
                className="index-pattern-react-select"
                placeholder="Select index..."
                loadOptions={this.loadOptions}
                value={this.props.value}
                resetValue={''}
                onChange={this.props.onChange} 
                inputProps={{ id : cId }}
              />
            </EuiFormRow>
        ); 
    }
}

CastIndexSelect.propTypes = {
    value: PropTypes.string,
    onChange: PropTypes.func.isRequired,
    getIndexPatterns: PropTypes.func.isRequired,
    controlIndex: PropTypes.number.isRequired,
    defaultValue: PropTypes.string
}
