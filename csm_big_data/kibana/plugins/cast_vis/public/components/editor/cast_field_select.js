import PropTypes from 'prop-types';
import React, { Component } from 'react';
import Select from 'react-select';

import {
    EuiFormRow
} from '@elastic/eui';


export class CastFieldSelect extends Component
{

    constructor(props){
        super(props);
        this.indexPatternID = props.indexPatternId;
    }
    
    render() { 
        return (
            <EuiFormRow
                label="Fields"
            >
                <Select
                    name="fields"
                    multi
                    value={this.props.value}
                    options={this.props.fieldList}
                    closeMenuOnSelect={false}
                    onChange={this.props.onChange}
                />

            </EuiFormRow>
        );
    }
}


CastFieldSelect.propTypes = {
    value: PropTypes.array,
    searchIndex: PropTypes.number.isRequired,
    fieldList: PropTypes.array.isRequired,
    onChange: PropTypes.func.isRequired

}


