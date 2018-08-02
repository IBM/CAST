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
        this.props.getIndexPatterns(input).then((indexPatterns) =>{
         const options = indexPatterns.map((obj) => {
          return {
           label: obj.attributes.title,
           value: obj.id
          };
         });
         callback(null, {options:options});
        });
    }

    render() { 
        const cId = `castIndexSelect-${this.props.controlIndex}`;
        return (
            <EuiFormRow
                id={cId}
                label="Index"
            >
              <Select.Async
                className="index-pattern-react-select"
                placeholder="Select index..."
                value={this.props.value}
                defaultValue={this.props.defaultValue}
                resetValue={this.props.defaultValue}
                loadOptions={this.loadOptions}
                onChange={this.props.onChange} 
                inputProps={{ id:cId }}
              />
            </EuiFormRow>
        ); 
    }
}

CastIndexSelect.propTypes = {
    value: PropTypes.string,
    onChange: PropTypes.func.isRequired,
    controlIndex: PropTypes.number.isRequired,
    getIndexPatterns: PropTypes.func.isRequired,
    defaultValue: PropTypes.string
}
